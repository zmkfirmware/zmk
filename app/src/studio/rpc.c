/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "msg_framing.h"
#include "rpc_tx.h"

#include <pb_decode.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/debug/thread_analyzer.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(zmk_studio, CONFIG_ZMK_STUDIO_LOG_LEVEL);

#include <zmk/endpoints.h>
#include <zmk/event_manager.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/studio/core.h>
#include <zmk/studio/rpc.h>

ZMK_EVENT_IMPL(zmk_studio_rpc_notification);

static struct zmk_rpc_subsystem *find_subsystem_for_choice(uint8_t choice) {
    STRUCT_SECTION_FOREACH(zmk_rpc_subsystem, sub) {
        if (sub->subsystem_choice == choice) {
            return sub;
        }
    }

    return NULL;
}

zmk_studio_Response zmk_rpc_subsystem_delegate_to_subs(const struct zmk_rpc_subsystem *subsys,
                                                       const zmk_studio_Request *req,
                                                       uint8_t which_req) {
    LOG_DBG("Got subsystem func for %d", subsys->subsystem_choice);

    for (int i = subsys->handlers_start_index; i <= subsys->handlers_end_index; i++) {
        struct zmk_rpc_subsystem_handler *sub_handler;
        STRUCT_SECTION_GET(zmk_rpc_subsystem_handler, i, &sub_handler);
        if (sub_handler->request_choice == which_req) {
            if (sub_handler->security == ZMK_STUDIO_RPC_HANDLER_SECURED &&
                zmk_studio_core_get_lock_state() != ZMK_STUDIO_CORE_LOCK_STATE_UNLOCKED) {
                return ZMK_RPC_RESPONSE(meta, simple_error,
                                        zmk_meta_ErrorConditions_UNLOCK_REQUIRED);
            }
            return sub_handler->func(req);
        }
    }
    LOG_ERR("No handler func found for %d", which_req);
    return ZMK_RPC_RESPONSE(meta, simple_error, zmk_meta_ErrorConditions_RPC_NOT_FOUND);
}

static zmk_studio_Response handle_request(const zmk_studio_Request *req) {
    zmk_studio_core_reschedule_lock_timeout();
    struct zmk_rpc_subsystem *sub = find_subsystem_for_choice(req->which_subsystem);
    if (!sub) {
        LOG_WRN("No subsystem found for choice %d", req->which_subsystem);
        return ZMK_RPC_RESPONSE(meta, simple_error, zmk_meta_ErrorConditions_RPC_NOT_FOUND);
    }

    zmk_studio_Response resp = sub->func(sub, req);
    resp.type.request_response.request_id = req->request_id;

    return resp;
}

#if IS_ENABLED(CONFIG_ZMK_STUDIO_TRANSPORT_BLE)
#include <zephyr/bluetooth/l2cap.h>
// A GATT write is accepted atomically; keep room for at least one ATT write.
RING_BUF_DECLARE(rpc_rx_buf, MAX(CONFIG_ZMK_STUDIO_RPC_RX_BUF_SIZE, BT_L2CAP_RX_MTU - 3));
#else
RING_BUF_DECLARE(rpc_rx_buf, CONFIG_ZMK_STUDIO_RPC_RX_BUF_SIZE);
#endif

static K_SEM_DEFINE(rpc_rx_sem, 0, 1);

static enum studio_framing_state rpc_framing_state;
static struct k_spinlock rx_lock;
static enum zmk_transport rx_transport;
static uint32_t rx_session;

static K_MUTEX_DEFINE(rpc_transport_mutex);
static struct zmk_rpc_transport *selected_transport;

int zmk_rpc_rx_put(enum zmk_transport source, const uint8_t *data, size_t len) {
    int rc = 0;
    k_spinlock_key_t key = k_spin_lock(&rx_lock);
    if (source != rx_transport) {
        rc = -ENOTCONN;
    } else if (len > ring_buf_space_get(&rpc_rx_buf)) {
        rc = -ENOSPC;
    } else {
        ring_buf_put(&rpc_rx_buf, data, len);
    }
    k_spin_unlock(&rx_lock, key);
    if (!rc) {
        k_sem_give(&rpc_rx_sem);
    }
    return rc;
}

static void select_session(const struct zmk_rpc_transport *transport) {
    k_spinlock_key_t key = k_spin_lock(&rx_lock);
    studio_tx_select(transport);
    rx_session = studio_tx_session();
    rx_transport = transport ? transport->transport : ZMK_TRANSPORT_NONE;
    ring_buf_reset(&rpc_rx_buf);
    k_spin_unlock(&rx_lock, key);
    k_sem_give(&rpc_rx_sem);
}

static bool rpc_read_cb(pb_istream_t *stream, uint8_t *buf, size_t count) {
    uint32_t expected = *(uint32_t *)stream->state;
    while (count) {
        uint8_t byte;
        k_spinlock_key_t key = k_spin_lock(&rx_lock);
        bool current = rx_session == expected;
        bool ready = current && ring_buf_get(&rpc_rx_buf, &byte, 1) == 1;
        k_spin_unlock(&rx_lock, key);
        if (!current) {
            return false;
        }
        if (!ready) {
            k_sem_take(&rpc_rx_sem, K_FOREVER);
            continue;
        }
        if (studio_framing_process_byte(&rpc_framing_state, byte)) {
            *buf++ = byte;
            count--;
        } else if (rpc_framing_state == FRAMING_STATE_EOF) {
            stream->bytes_left = 0;
            return false;
        }
    }
    return true;
}

struct queued_notification {
    uint32_t session;
    zmk_studio_Notification notification;
};

K_MSGQ_DEFINE(notifications, sizeof(struct queued_notification), 8, 4);

static void notification_main(void *a, void *b, void *c) {
    struct queued_notification item;
    while (true) {
        k_msgq_get(&notifications, &item, K_FOREVER);
        zmk_studio_Response response = {.which_type = zmk_studio_Response_notification_tag,
                                        .type.notification = item.notification};
        studio_tx_send(&response, item.session);
    }
}

K_THREAD_DEFINE(studio_notify_thread, CONFIG_ZMK_STUDIO_RPC_THREAD_STACK_SIZE, notification_main,
                NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

static void rpc_main(void) {
    for (;;) {
        uint32_t request_session = studio_tx_session();
        pb_istream_t stream = {
            .callback = rpc_read_cb, .state = &request_session, .bytes_left = SIZE_MAX};
        zmk_studio_Request req = zmk_studio_Request_init_zero;
#if IS_ENABLED(CONFIG_THREAD_ANALYZER)
        thread_analyzer_print(0);
#endif // IS_ENABLED(CONFIG_THREAD_ANALYZER)
        bool status = pb_decode(&stream, &zmk_studio_Request_msg, &req);

        rpc_framing_state = FRAMING_STATE_IDLE;

        if (status && request_session == studio_tx_session()) {
            zmk_studio_Response resp = handle_request(&req);

            int err = studio_tx_send(&resp, request_session);
#if IS_ENABLED(CONFIG_THREAD_ANALYZER)
            thread_analyzer_print(0);
#endif // IS_ENABLED(CONFIG_THREAD_ANALYZER)
            if (err < 0) {
                LOG_ERR("Failed to send the RPC response %d", err);
            }
        } else {
            LOG_DBG("Decode failed");
        }
    }
}

K_THREAD_DEFINE(studio_rpc_thread, CONFIG_ZMK_STUDIO_RPC_THREAD_STACK_SIZE, rpc_main, NULL, NULL,
                NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);

static void refresh_selected_transport(void) {
    enum zmk_transport transport = zmk_endpoint_get_selected().transport;

    k_mutex_lock(&rpc_transport_mutex, K_FOREVER);

    if (selected_transport && selected_transport->transport == transport) {
        select_session(selected_transport);
        goto exit_refresh;
    }

    select_session(NULL);
    if (selected_transport) {
        if (selected_transport->rx_stop) {
            selected_transport->rx_stop();
        }
        selected_transport = NULL;
#if IS_ENABLED(CONFIG_ZMK_STUDIO_LOCK_ON_DISCONNECT)
        zmk_studio_core_lock();
#endif
    }

    STRUCT_SECTION_FOREACH(zmk_rpc_transport, t) {
        if (t->transport == transport) {
            selected_transport = t;
            select_session(t);
            if (selected_transport->rx_start) {
                selected_transport->rx_start();
            }
            break;
        }
    }

    if (!selected_transport) {
        LOG_WRN("Failed to select a transport!");
    }

exit_refresh:
    k_mutex_unlock(&rpc_transport_mutex);
}

static int zmk_rpc_init(void) {
    int prev_choice = -1;
    struct zmk_rpc_subsystem *prev_sub = NULL;
    int i = 0;

    STRUCT_SECTION_FOREACH(zmk_rpc_subsystem_handler, handler) {
        struct zmk_rpc_subsystem *sub = find_subsystem_for_choice(handler->subsystem_choice);

        __ASSERT(sub != NULL, "RPC Handler for unknown subsystem choice %d",
                 handler->subsystem_choice);

        if (prev_choice < 0) {
            sub->handlers_start_index = i;
        } else if ((prev_choice != handler->subsystem_choice) && prev_sub) {
            prev_sub->handlers_end_index = i - 1;
            sub->handlers_start_index = i;
        }

        prev_choice = handler->subsystem_choice;
        prev_sub = sub;
        i++;
    }

    if (prev_sub) {
        prev_sub->handlers_end_index = i - 1;
    }

    refresh_selected_transport();

    return 0;
}

SYS_INIT(zmk_rpc_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static int studio_rpc_listener_cb(const zmk_event_t *eh) {
    struct zmk_endpoint_changed *ep_changed = as_zmk_endpoint_changed(eh);
    if (ep_changed) {
        refresh_selected_transport();
        return ZMK_EV_EVENT_BUBBLE;
    }

    struct zmk_studio_rpc_notification *rpc_notify = as_zmk_studio_rpc_notification(eh);
    if (rpc_notify) {
        struct queued_notification item = {.session = studio_tx_session(),
                                           .notification = rpc_notify->notification};
        if (k_msgq_put(&notifications, &item, K_NO_WAIT) < 0) {
            LOG_WRN("Studio notification queue full");
        }
        return ZMK_EV_EVENT_BUBBLE;
    }

    zmk_studio_Notification n = zmk_studio_Notification_init_zero;
    STRUCT_SECTION_FOREACH(zmk_rpc_event_mapper, mapper) {
        int ret = mapper->func(eh, &n);
        if (ret >= 0) {
            raise_zmk_studio_rpc_notification(
                (struct zmk_studio_rpc_notification){.notification = n});
            break;
        }
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(studio_rpc, studio_rpc_listener_cb);
ZMK_SUBSCRIPTION(studio_rpc, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(studio_rpc, zmk_studio_rpc_notification);
