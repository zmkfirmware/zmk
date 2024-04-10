/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/ring_buffer.h>

#include <proto/zmk/studio.pb.h>

#include <zmk/endpoints_types.h>
#include <zmk/event_manager.h>
#include <zmk/studio/core.h>

enum zmk_studio_rpc_handler_security {
    ZMK_STUDIO_RPC_HANDLER_SECURED,
    ZMK_STUDIO_RPC_HANDLER_UNSECURED,
};

struct zmk_studio_rpc_notification {
    zmk_studio_Notification notification;
};

ZMK_EVENT_DECLARE(zmk_studio_rpc_notification);

struct zmk_rpc_subsystem;

typedef zmk_studio_Response(subsystem_func)(const struct zmk_rpc_subsystem *subsys,
                                            const zmk_studio_Request *req);

typedef zmk_studio_Response(rpc_func)(const zmk_studio_Request *neq);

/**
 * @brief An RPC subsystem is a cohesive collection of related RPCs. A specific RPC is identified by
 *        the pair or subsystem and request identifiers. This struct is the high level entity to
 *        aggregate all the possible handler functions for the request in the given subsystem.
 */
struct zmk_rpc_subsystem {
    subsystem_func *func;
    uint16_t handlers_start_index;
    uint16_t handlers_end_index;
    uint8_t subsystem_choice;
};

/**
 * @brief An entry for a specific handler function in a given subsystem, including metadata
 * indicating if the particular handler requires the device be unlock in order to be invoked.
 */
struct zmk_rpc_subsystem_handler {
    rpc_func *func;
    uint8_t subsystem_choice;
    uint8_t request_choice;
    enum zmk_studio_rpc_handler_security security;
};

typedef int (*zmk_rpc_subsystem_settings_reset_func)(void);

struct zmk_rpc_subsystem_settings_reset {
    zmk_rpc_subsystem_settings_reset_func callback;
};

/**
 * @brief Generate a "meta" subsystem response indicating an "empty" response to an RPC request.
 */
#define ZMK_RPC_NO_RESPONSE() ZMK_RPC_RESPONSE(meta, no_response, true)

/**
 * @brief Generate a "meta" subsystem response with one of a few possible simple error responses.
 * @see   https://github.com/zmkfirmware/zmk-studio-messages/blob/main/proto/zmk/meta.proto#L5
 */
#define ZMK_RPC_SIMPLE_ERR(type)                                                                   \
    ZMK_RPC_RESPONSE(meta, simple_error, zmk_meta_ErrorConditions_##type)

/**
 * @brief Register an RPC subsystem to aggregate handlers for request to that subsystem.
 * @param prefix The identifier for the subsystem, e.g. `core`, `keymap`, etc.
 * @see   https://github.com/zmkfirmware/zmk-studio-messages/blob/main/proto/zmk/studio.proto#L15
 */
#define ZMK_RPC_SUBSYSTEM(prefix)                                                                  \
    zmk_studio_Response subsystem_func_##prefix(const struct zmk_rpc_subsystem *subsys,            \
                                                const zmk_studio_Request *req) {                   \
        uint8_t which_req = req->subsystem.prefix.which_request_type;                              \
        return zmk_rpc_subsystem_delegate_to_subs(subsys, req, which_req);                         \
    }                                                                                              \
    STRUCT_SECTION_ITERABLE(zmk_rpc_subsystem, prefix##_subsystem) = {                             \
        .func = subsystem_func_##prefix,                                                           \
        .subsystem_choice = zmk_studio_Request_##prefix##_tag,                                     \
    };

/**
 * @brief Register an RPC subsystem handler handler a specific request within the subsystem.
 * @param prefix The identifier for the subsystem, e.g. `core`, `keymap`, etc.
 * @param request_id The identifier for the request ID, e.g. `save_changes`.
 * @param _secured Whether the handler requires the device be unlocked to allow invocation.
 *
 * @note  A function with a name matching the request_id must be in-scope and will be used as the
 *        the callback handler. The function must have a signature of
 *        zmk_studio_Response (*func)(const zmk_studio_Request*)
 */
#define ZMK_RPC_SUBSYSTEM_HANDLER(prefix, request_id, _security)                                   \
    STRUCT_SECTION_ITERABLE(zmk_rpc_subsystem_handler,                                             \
                            prefix##_subsystem_handler_##request_id) = {                           \
        .func = request_id,                                                                        \
        .subsystem_choice = zmk_studio_Request_##prefix##_tag,                                     \
        .request_choice = zmk_##prefix##_Request_##request_id##_tag,                               \
        .security = _security,                                                                     \
    };

#define ZMK_RPC_SUBSYSTEM_SETTINGS_RESET(prefix, _callback)                                        \
    STRUCT_SECTION_ITERABLE(zmk_rpc_subsystem_settings_reset, _##prefix##_settings_reset) = {      \
        .callback = _callback,                                                                     \
    };

#define ZMK_RPC_SUBSYSTEM_SETTINGS_RESET_FOREACH(_var)                                             \
    STRUCT_SECTION_FOREACH(zmk_rpc_subsystem_settings_reset, _var)

/**
 * @brief Create a zmk_studio_Notification struct for the given subsystem and type, including
          initialization of the inner fields.
 * @param subsys The identifier for the subsystem, e.g. `core`, `keymap`, etc.
 * @param _type The identifier for the notification type in that subsystem, e.g.
 `unsaved_changes_status_changed`.
 *
 * @see   example
 https://github.com/zmkfirmware/zmk-studio-messages/blob/main/proto/zmk/keymap.proto#L41C14-L41C44
 */
#define ZMK_RPC_NOTIFICATION(subsys, _type, ...)                                                   \
    ((zmk_studio_Notification){                                                                    \
        .which_subsystem = zmk_studio_Notification_##subsys##_tag,                                 \
        .subsystem =                                                                               \
            {                                                                                      \
                .subsys =                                                                          \
                    {                                                                              \
                        .which_notification_type = zmk_##subsys##_Notification_##_type##_tag,      \
                        .notification_type = {._type = __VA_ARGS__},                               \
                    },                                                                             \
            },                                                                                     \
    })

/**
 * @brief Create a zmk_studio_Response struct for the given subsystem and type, including
          initialization of the inner fields.
 * @param subsys The identifier for the subsystem, e.g. `core`, `keymap`, etc.
 * @param _type The identifier for the response type in that subsystem, e.g. `get_keymap`.
 *
 * @see   example
 https://github.com/zmkfirmware/zmk-studio-messages/blob/main/proto/zmk/keymap.proto#L24
 */
#define ZMK_RPC_RESPONSE(subsys, _type, ...)                                                       \
    ((zmk_studio_Response){                                                                        \
        .which_type = zmk_studio_Response_request_response_tag,                                    \
        .type =                                                                                    \
            {                                                                                      \
                .request_response =                                                                \
                    {                                                                              \
                        .which_subsystem = zmk_studio_RequestResponse_##subsys##_tag,              \
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

typedef int(zmk_rpc_event_mapper_cb)(const zmk_event_t *ev, zmk_studio_Notification *n);

struct zmk_rpc_event_mapper {
    zmk_rpc_event_mapper_cb *func;
};

/**
 * @brief A single ZMK event listener is registered that will listen for events and map them to
 *        RPC notifications to be sent to the connected client. This macro adds additional
 * subscriptions to that one single registered listener.
 * @param _t The ZMK event type.
 */
#define ZMK_RPC_EVENT_MAPPER_ADD_LISTENER(_t) ZMK_SUBSCRIPTION(studio_rpc, _t)

/**
 * @brief Register a mapping function that can selectively map a given internal ZMK event type into
 *        a possible zmk_studio_Notification type.
 * @param name A unique identifier for the mapper. Often a subsystem identifier like `core` is used.
 * @param _func The `zmk_rpc_event_mapper_cb` function used to map the internal event type.
 */
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

zmk_studio_Response zmk_rpc_subsystem_delegate_to_subs(const struct zmk_rpc_subsystem *subsys,
                                                       const zmk_studio_Request *req,
                                                       uint8_t which_req);

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
