
#pragma once

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/ring_buffer.h>

#include <proto/zmk/studio.pb.h>

#include <zmk/endpoints_types.h>
#include <zmk/event_manager.h>
#include <zmk/studio/core.h>

struct zmk_studio_rpc_notification {
    zmk_Notification notification;
};

ZMK_EVENT_DECLARE(zmk_studio_rpc_notification);

struct zmk_rpc_subsystem;

typedef zmk_Response(subsystem_func)(const struct zmk_rpc_subsystem *subsys,
                                     const zmk_Request *req);

typedef zmk_Response(rpc_func)(const zmk_Request *neq);

struct zmk_rpc_subsystem {
    subsystem_func *func;
    uint16_t handlers_start_index;
    uint16_t handlers_end_index;
    uint8_t subsystem_choice;
};

struct zmk_rpc_subsystem_handler {
    rpc_func *func;
    uint8_t subsystem_choice;
    uint8_t request_choice;
    bool secured;
};

#define ZMK_RPC_NO_RESPONSE() ZMK_RPC_RESPONSE(meta, no_response, true)
#define ZMK_RPC_SIMPLE_ERR(type)                                                                   \
    ZMK_RPC_RESPONSE(meta, simple_error, zmk_meta_ErrorConditions_##type)

#define ZMK_RPC_SUBSYSTEM(prefix)                                                                  \
    zmk_Response subsystem_func_##prefix(const struct zmk_rpc_subsystem *subsys,                   \
                                         const zmk_Request *req) {                                 \
        uint8_t which_req = req->subsystem.prefix.which_request_type;                              \
        LOG_DBG("Got subsystem func for %d", subsys->subsystem_choice);                            \
                                                                                                   \
        for (int i = subsys->handlers_start_index; i <= subsys->handlers_end_index; i++) {         \
            struct zmk_rpc_subsystem_handler *sub_handler;                                         \
            STRUCT_SECTION_GET(zmk_rpc_subsystem_handler, i, &sub_handler);                        \
            if (sub_handler->request_choice == which_req) {                                        \
                if (sub_handler->secured &&                                                        \
                    zmk_studio_core_get_lock_state() != ZMK_STUDIO_CORE_LOCK_STATE_UNLOCKED) {     \
                    return ZMK_RPC_RESPONSE(meta, simple_error,                                    \
                                            zmk_meta_ErrorConditions_UNLOCK_REQUIRED);             \
                }                                                                                  \
                return sub_handler->func(req);                                                     \
            }                                                                                      \
        }                                                                                          \
        LOG_ERR("No handler func found for %d", which_req);                                        \
        return ZMK_RPC_RESPONSE(meta, simple_error, zmk_meta_ErrorConditions_RPC_NOT_FOUND);       \
    }                                                                                              \
    STRUCT_SECTION_ITERABLE(zmk_rpc_subsystem, prefix##_subsystem) = {                             \
        .func = subsystem_func_##prefix,                                                           \
        .subsystem_choice = zmk_Request_##prefix##_tag,                                            \
    };

#define ZMK_RPC_SUBSYSTEM_HANDLER(prefix, request_id, _secured)                                    \
    STRUCT_SECTION_ITERABLE(zmk_rpc_subsystem_handler,                                             \
                            prefix##_subsystem_handler_##request_id) = {                           \
        .func = request_id,                                                                        \
        .subsystem_choice = zmk_Request_##prefix##_tag,                                            \
        .request_choice = zmk_##prefix##_Request_##request_id##_tag,                               \
        .secured = _secured,                                                                       \
    };

#define ZMK_RPC_NOTIFICATION(subsys, _type, ...)                                                   \
    ((zmk_Notification){                                                                           \
        .which_subsystem = zmk_Notification_##subsys##_tag,                                        \
        .subsystem =                                                                               \
            {                                                                                      \
                .subsys =                                                                          \
                    {                                                                              \
                        .which_notification_type = zmk_##subsys##_Notification_##_type##_tag,      \
                        .notification_type = {._type = __VA_ARGS__},                               \
                    },                                                                             \
            },                                                                                     \
    })

#define ZMK_RPC_RESPONSE(subsys, _type, ...)                                                       \
    ((zmk_Response){                                                                               \
        .which_type = zmk_Response_request_response_tag,                                           \
        .type =                                                                                    \
            {                                                                                      \
                .request_response =                                                                \
                    {                                                                              \
                        .which_subsystem = zmk_RequestResponse_##subsys##_tag,                     \
                        .subsystem =                                                               \
                            {                                                                      \
                                .subsys =                                                          \
                                    {                                                              \
                                        .which_response_type =                                     \
                                            zmk_##subsys##_Response_##_type##_tag,                 \
                                        .response_type = {._type = __VA_ARGS__},                   \
                                    },                                                             \
                            },                                                                     \
                    },                                                                             \
            },                                                                                     \
    })

typedef int(zmk_rpc_event_mapper_cb)(const zmk_event_t *ev, zmk_Notification *n);

struct zmk_rpc_event_mapper {
    zmk_rpc_event_mapper_cb *func;
};

#define ZMK_RPC_EVENT_MAPPER_ADD_LISTENER(_t) ZMK_SUBSCRIPTION(studio_rpc, _t)

#define ZMK_RPC_EVENT_MAPPER(name, _func, ...)                                                     \
    FOR_EACH_NONEMPTY_TERM(ZMK_RPC_EVENT_MAPPER_ADD_LISTENER, (;), __VA_ARGS__)                    \
    STRUCT_SECTION_ITERABLE(zmk_rpc_event_mapper, name) = {                                        \
        .func = _func,                                                                             \
    };

typedef int (*zmk_rpc_rx_start_stop_func)(void);

typedef void (*zmk_rpc_tx_buffer_notify_func)(struct ring_buf *buf, size_t added, bool message_done,
                                              void *user_data);
typedef void *(*zmk_rpc_tx_user_data_func)(void);

struct zmk_rpc_transport {
    enum zmk_transport transport;

    zmk_rpc_tx_user_data_func tx_user_data;
    zmk_rpc_tx_buffer_notify_func tx_notify;
    zmk_rpc_rx_start_stop_func rx_start;
    zmk_rpc_rx_start_stop_func rx_stop;
};

struct ring_buf *zmk_rpc_get_tx_buf(void);
struct ring_buf *zmk_rpc_get_rx_buf(void);
void zmk_rpc_rx_notify(void);

#define ZMK_RPC_TRANSPORT(name, _transport, _rx_start, _rx_stop, _tx_user_data, _tx_notify)        \
    STRUCT_SECTION_ITERABLE(zmk_rpc_transport, name) = {                                           \
        .transport = _transport,                                                                   \
        .rx_start = _rx_start,                                                                     \
        .rx_stop = _rx_stop,                                                                       \
        .tx_user_data = _tx_user_data,                                                             \
        .tx_notify = _tx_notify,                                                                   \
    }
