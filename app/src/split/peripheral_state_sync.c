/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/split/peripheral.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
#include <zmk/hid_indicators.h>
#include <zmk/events/hid_indicators_changed.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_LAYER_STATE)
#include <zmk/keymap.h>
#include <zmk/events/layer_state_changed.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE)
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY)
#include <zmk/events/battery_state_changed.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// The type of a state sync payload, such as PAYLOAD_TYPE(set_endpoint).
#define PAYLOAD_TYPE(_field)                                                                       \
    __typeof__(((struct zmk_split_transport_central_command *)0)->data._field)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

static struct {
    bool known;
    zmk_hid_indicators_t indicators;
} hid_indicators;

zmk_hid_indicators_t zmk_hid_indicators_get_current_profile(void) {
    return hid_indicators.indicators;
}

static void handle_hid_indicators(const struct zmk_split_transport_central_command *cmd) {
    if (hid_indicators.known &&
        hid_indicators.indicators == cmd->data.set_hid_indicators.indicators) {
        return;
    }

    hid_indicators.known = true;
    hid_indicators.indicators = cmd->data.set_hid_indicators.indicators;
    LOG_DBG("Raising HID indicators changed event: %x", hid_indicators.indicators);

    raise_zmk_hid_indicators_changed(
        (struct zmk_hid_indicators_changed){.indicators = hid_indicators.indicators});
}

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_LAYER_STATE)

#define LAYER_STATE_BITS (sizeof(zmk_keymap_layers_state_t) * 8)

static struct {
    bool known;
    zmk_keymap_layers_state_t state;
    zmk_keymap_layer_id_t default_layer;
    zmk_keymap_layer_index_t highest_index;
    zmk_keymap_layer_id_t highest_id;
} layers;

// Only the name of the highest active layer is synced.
static struct {
    zmk_keymap_layer_id_t id;
    char name[ZMK_SPLIT_TRANSPORT_LAYER_NAME_LEN];
} layer_name = {.id = ZMK_KEYMAP_LAYER_ID_INVAL};

zmk_keymap_layer_index_t zmk_keymap_highest_layer_active(void) { return layers.highest_index; }

bool zmk_keymap_layer_active(zmk_keymap_layer_id_t layer) {
    return layer == layers.default_layer ||
           (layer < LAYER_STATE_BITS && (layers.state & BIT(layer)));
}

zmk_keymap_layer_id_t zmk_keymap_layer_index_to_id(zmk_keymap_layer_index_t layer_index) {
    // Only the highest active layer's id is synced; ids equal indexes unless layers are reordered.
    return layer_index == layers.highest_index ? layers.highest_id : layer_index;
}

const char *zmk_keymap_layer_name(zmk_keymap_layer_id_t layer) {
    return layer == layer_name.id ? layer_name.name : NULL;
}

static void handle_layer_name(const struct zmk_split_transport_central_command *cmd) {
    zmk_keymap_layer_id_t id = cmd->data.set_layer_name.layer_id;
    bool changed = id != layer_name.id || strncmp(layer_name.name, cmd->data.set_layer_name.name,
                                                  sizeof(layer_name.name) - 1) != 0;

    layer_name.id = id;
    strncpy(layer_name.name, cmd->data.set_layer_name.name, sizeof(layer_name.name) - 1);

    // A name usually precedes the layer state that activates its layer. A rename of the layer
    // already shown has no state change, so announce it the same way.
    if (changed && layers.known && id == layers.highest_id && id < LAYER_STATE_BITS) {
        raise_layer_state_changed(id, true, false);
    }
}

// Returns whether it raised an event.
static bool handle_layer_state(const struct zmk_split_transport_central_command *cmd) {
    zmk_keymap_layers_state_t old_state = layers.state;
    zmk_keymap_layer_id_t old_highest_id = layers.highest_id;
    bool changed = !layers.known || old_state != cmd->data.set_layer_state.state ||
                   layers.default_layer != cmd->data.set_layer_state.default_layer ||
                   layers.highest_index != cmd->data.set_layer_state.highest_index ||
                   old_highest_id != cmd->data.set_layer_state.highest_id;

    if (!changed) {
        return false;
    }

    layers.state = cmd->data.set_layer_state.state;
    layers.default_layer = cmd->data.set_layer_state.default_layer;
    layers.highest_index = cmd->data.set_layer_state.highest_index;
    layers.highest_id = cmd->data.set_layer_state.highest_id;

    const char *name = zmk_keymap_layer_name(layers.highest_id);
    LOG_DBG("layer state %08x highest %d/%d, name '%s'", layers.state, layers.highest_index,
            layers.highest_id, name ? name : "");

    zmk_keymap_layers_state_t raise = old_state ^ layers.state;
    if ((!layers.known || old_highest_id != layers.highest_id) &&
        layers.highest_id < LAYER_STATE_BITS) {
        raise |= BIT(layers.highest_id);
    }
    layers.known = true;

    for (zmk_keymap_layer_id_t id = 0; id < LAYER_STATE_BITS; id++) {
        if (raise & BIT(id)) {
            raise_layer_state_changed(id, zmk_keymap_layer_active(id), false);
        }
    }

    return raise != 0;
}

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_LAYER_STATE)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE)

static struct {
    bool known;
    uint8_t index;
    uint8_t flags;
} ble_profile;

int zmk_ble_active_profile_index(void) { return ble_profile.index; }

bool zmk_ble_active_profile_is_connected(void) {
    return ble_profile.flags & ZMK_SPLIT_TRANSPORT_BLE_PROFILE_FLAG_CONNECTED;
}

bool zmk_ble_active_profile_is_open(void) {
    return !(ble_profile.flags & ZMK_SPLIT_TRANSPORT_BLE_PROFILE_FLAG_BONDED);
}

static void handle_ble_profile(const struct zmk_split_transport_central_command *cmd) {
    if (ble_profile.known && ble_profile.index == cmd->data.set_ble_profile.index &&
        ble_profile.flags == cmd->data.set_ble_profile.flags) {
        return;
    }

    ble_profile.known = true;
    ble_profile.index = cmd->data.set_ble_profile.index;
    ble_profile.flags = cmd->data.set_ble_profile.flags;
    LOG_DBG("ble profile %d connected %d bonded %d", ble_profile.index,
            zmk_ble_active_profile_is_connected(), !zmk_ble_active_profile_is_open());

    // The profile details (address, name) stay on the central.
    raise_zmk_ble_active_profile_changed(
        (struct zmk_ble_active_profile_changed){.index = ble_profile.index, .profile = NULL});
}

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE)

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY)

static struct {
    bool known;
    PAYLOAD_TYPE(set_central_battery) payload;
} central_battery;

int zmk_split_peripheral_get_central_battery_level(uint8_t *level) {
    if (!central_battery.known) {
        return -ENODATA;
    }

    *level = central_battery.payload.state_of_charge;
    return 0;
}

static void handle_central_battery(const struct zmk_split_transport_central_command *cmd) {
    if (central_battery.known && memcmp(&central_battery.payload, &cmd->data.set_central_battery,
                                        sizeof(central_battery.payload)) == 0) {
        return;
    }

    central_battery.known = true;
    central_battery.payload = cmd->data.set_central_battery;
    LOG_DBG("central battery %d%% usb %d", central_battery.payload.state_of_charge,
            central_battery.payload.usb_powered);

    raise_zmk_central_battery_state_changed((struct zmk_central_battery_state_changed){
        .state_of_charge = central_battery.payload.state_of_charge,
        .usb_powered = central_battery.payload.usb_powered,
    });
}

#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY)

int zmk_split_peripheral_state_sync_handle(const struct zmk_split_transport_central_command *cmd) {
    switch (cmd->type) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_HID_INDICATORS:
        handle_hid_indicators(cmd);
        break;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_LAYER_STATE)
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_LAYER_NAME:
        handle_layer_name(cmd);
        break;
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_LAYER_STATE:
        handle_layer_state(cmd);
        break;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_LAYER_STATE)
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE)
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_BLE_PROFILE:
        handle_ble_profile(cmd);
        break;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE)
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY)
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_CENTRAL_BATTERY:
        handle_central_battery(cmd);
        break;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY)
    default:
        return -ENOTSUP;
    }

    return 0;
}

// While the central is unreachable, every domain returns to its boot state. A zeroed payload equals
// the boot value, so only values that change raise events, and the next value from the central
// raises again, as after boot.
static void reset_work_cb(struct k_work *work) {
    // Unused when the central battery is the only domain.
    static const struct zmk_split_transport_central_command boot_state __maybe_unused;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    if (hid_indicators.known) {
        handle_hid_indicators(&boot_state);
        hid_indicators.known = false;
    }
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_LAYER_STATE)
    bool had_name = layer_name.id != ZMK_KEYMAP_LAYER_ID_INVAL;
    // Cleared first, so listeners of the layer events below find no name.
    layer_name.id = ZMK_KEYMAP_LAYER_ID_INVAL;
    layer_name.name[0] = '\0';
    if (layers.known) {
        // Without a layer change, announce the highest layer anyway to drop the shown name.
        if (!handle_layer_state(&boot_state) && had_name) {
            raise_layer_state_changed(layers.highest_id, zmk_keymap_layer_active(layers.highest_id),
                                      false);
        }
        layers.known = false;
    }
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_LAYER_STATE)
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE)
    if (ble_profile.known) {
        handle_ble_profile(&boot_state);
        ble_profile.known = false;
    }
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_BLE_PROFILE)
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY)
    // The level is unknown again; the event has no way to say so, so none is raised.
    central_battery.known = false;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_BATTERY)
}

static K_WORK_DEFINE(reset_work, reset_work_cb);

// Only BLE peripherals raise this event; wired ones keep the last received state.
static int state_sync_status_listener(const zmk_event_t *eh) {
    const struct zmk_split_peripheral_status_changed *ev =
        as_zmk_split_peripheral_status_changed(eh);

    // The system work queue also handles received commands, so the reset cannot interleave with
    // them.
    if (ev && !ev->connected) {
        k_work_submit(&reset_work);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(split_peripheral_state_sync, state_sync_status_listener);
ZMK_SUBSCRIPTION(split_peripheral_state_sync, zmk_split_peripheral_status_changed);
