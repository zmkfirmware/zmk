/*
 * Copyright (c) 2025 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>

#include <zmk/stdlib.h>
#include <zmk/split/transport/peripheral.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/events/sensor_event.h>
#include <zmk/events/battery_state_changed.h>

#include <zephyr/init.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

const struct zmk_split_transport_peripheral *active_transport;

int zmk_split_transport_peripheral_command_handler(
    const struct zmk_split_transport_peripheral *transport,
    struct zmk_split_transport_central_command cmd) {
    LOG_DBG("");

    switch (cmd.type) {
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_INVOKE_BEHAVIOR: {
        struct zmk_behavior_binding binding = {
            .param1 = cmd.data.invoke_behavior.param1,
            .param2 = cmd.data.invoke_behavior.param2,
            .behavior_dev = cmd.data.invoke_behavior.behavior_dev,
        };
        LOG_DBG("%s with params %d %d: pressed? %d", binding.behavior_dev, binding.param1,
                binding.param2, cmd.data.invoke_behavior.state);
        struct zmk_behavior_binding_event event = {.position = cmd.data.invoke_behavior.position,
                                                   .timestamp = k_uptime_get()};
        int err;
        if (cmd.data.invoke_behavior.state > 0) {
            err = behavior_keymap_binding_pressed(&binding, event);
        } else {
            err = behavior_keymap_binding_released(&binding, event);
        }

        if (err) {
            LOG_ERR("Failed to invoke behavior %s: %d", binding.behavior_dev, err);
        }
    }
    default:
        LOG_WRN("Unhandled command type %d", cmd.type);
        return -ENOTSUP;
    }
    return 0;
}

int zmk_split_peripheral_report_event(const struct zmk_split_transport_peripheral_event *event) {
    if (!active_transport || !active_transport->api || !active_transport->api->report_event) {
        LOG_WRN("No active transport that supports reporting events!");
        return -ENODEV;
    }

    return active_transport->api->report_event(event);
}

static int select_first_available_transport(void) {
    // Transports are sorted by priority, so find the first
    // One that's available, and enable it. Any transport that
    // Doesn't support `get_status` is assumed to be always
    // available and fully connected.
    STRUCT_SECTION_FOREACH(zmk_split_transport_peripheral, t) {
        if (!t->api->get_status || t->api->get_status().available) {
            if (active_transport == t) {
                LOG_DBG("First available is already selected, moving on");
                return 0;
            }

            if (active_transport && active_transport->api->set_enabled) {
                int err = active_transport->api->set_enabled(false);
                if (err < 0) {
                    LOG_WRN("Error disabling previously selected split transport (%d)", err);
                }
            }

            active_transport = t;
            int err = 0;
            if (active_transport->api->set_enabled) {
                err = active_transport->api->set_enabled(true);
            }

            return err;
        }
    }

    return -ENODEV;
}

static int transport_status_changed_cb(const struct zmk_split_transport_peripheral *p,
                                       struct zmk_split_transport_status status) {
    if (p == active_transport) {
        LOG_DBG("Peripheral at %p changed status: enabled %d, available %d, connections %d", p,
                status.enabled, status.available, status.connections);
        if (status.connections == ZMK_SPLIT_TRANSPORT_CONNECTIONS_STATUS_DISCONNECTED) {
            LOG_DBG("Find us a new active transport!");

            return select_first_available_transport();
        }
    } else {
        select_first_available_transport();
    }

    return 0;
}

static int peripheral_init(void) {
    STRUCT_SECTION_FOREACH(zmk_split_transport_peripheral, t) {
        if (!t->api->set_status_callback) {
            continue;
        }

        t->api->set_status_callback(transport_status_changed_cb);
    }

    return select_first_available_transport();
}

SYS_INIT(peripheral_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

int split_peripheral_listener(const zmk_event_t *eh) {
    LOG_DBG("");
    const struct zmk_position_state_changed *pos_ev;
    if ((pos_ev = as_zmk_position_state_changed(eh)) != NULL) {
        struct zmk_split_transport_peripheral_event ev = {
            .type = ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_KEY_POSITION_EVENT,
            .data = {.key_position_event = {
                         .position = pos_ev->position,
                         .pressed = pos_ev->state,
                     }}};

        zmk_split_peripheral_report_event(&ev);
    }

#if ZMK_KEYMAP_HAS_SENSORS
    const struct zmk_sensor_event *sensor_ev;
    if ((sensor_ev = as_zmk_sensor_event(eh)) != NULL) {
        if (sensor_ev->channel_data_size != 1) {
            return -ENOTSUP;
        }

        struct zmk_split_transport_peripheral_event ev = {
            .type = ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_SENSOR_EVENT,
            .data = {.sensor_event = {
                         .channel_data = sensor_ev->channel_data[0],
                         .sensor_index = sensor_ev->sensor_index,
                     }}};

        zmk_split_peripheral_report_event(&ev);
    }
#endif /* ZMK_KEYMAP_HAS_SENSORS */

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    const struct zmk_battery_state_changed *battery_ev;
    if ((battery_ev = as_zmk_battery_state_changed(eh)) != NULL) {
        struct zmk_split_transport_peripheral_event ev = {
            .type = ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_BATTERY_EVENT,
            .data = {.battery_event = {
                         .level = battery_ev->state_of_charge,
                     }}};

        zmk_split_peripheral_report_event(&ev);
    }
#endif // IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(split_peripheral, split_peripheral_listener);
ZMK_SUBSCRIPTION(split_peripheral, zmk_position_state_changed);

#if ZMK_KEYMAP_HAS_SENSORS
ZMK_SUBSCRIPTION(split_peripheral, zmk_sensor_event);
#endif /* ZMK_KEYMAP_HAS_SENSORS */

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
ZMK_SUBSCRIPTION(split_peripheral, zmk_battery_state_changed);
#endif