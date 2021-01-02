---
title: Events
---

## Overview

ZMK uses events to decouple behaviours and to allow new functionality to be added to existing events.

## Subscribing to events

Subscribing to events uses a combination of the `ZMK_SUBSCRIPTION` and `ZMK_LISTENER` macros.

`ZMK_SUBSCRIPTION` tells the event manager that the module wants to subscribe to a particular type of event.

`ZMK_LISTENER` tells the event manager which method of the module to call when an event is raised on a subscription. When subscribing to multiple event types, you can check which event type was raised using the `is_<event_type>` helper methods (See example below).

:::note
The event manager expects the method to be called `<module>_listener` by convention.
:::

```
int behavior_hold_tap_listener(const struct zmk_event_header *eh) {
    if (is_position_state_changed(eh)) {
        return position_state_changed_listener(eh);
    } else if (is_keycode_state_changed(eh)) {
        return keycode_state_changed_listener(eh);
    }
    return 0;
}

ZMK_LISTENER(behavior_hold_tap, behavior_hold_tap_listener);
ZMK_SUBSCRIPTION(behavior_hold_tap, keycode_state_changed);
```

todo: Describe the return values from the listener methods.

## Events

### `activity_state_changed`

Fired by the activity monitor when the state changes. The monitor subscribes to `position_state_changed` and `sensor_event` events to keep track of activity.

- `ZMK_ACTIVITY_ACTIVE`
- `ZMK_ACTIVITY_IDLE` - When idle time is greater than `CONFIG_ZMK_IDLE_TIMEOUT `
- `ZMK_ACTIVITY_SLEEP` - When `CONFIG_ZMK_SLEEP` is enabled and idle time is greater than `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT`

### `battery_state_changed`

Fired every minute with an update of the battery charge status. Event contains the battery percentage.

### `ble_active_profile_changed`

Fired whenever the current Bluetooth profile changes. Event contains the index of the new profile.

### `position_state_changed`

Fired whenever there is a change in state of any key after a matrix scan.

The column/row data coming from the matrix is converted into an integer key `position` representing the 0 based index of the key that was pressed. Positions go from left to right, top to bottom, starting from 0.

That `state` bool represents whether the key is pressed or not.

`keymap` subscribes to these events, processes them against the configured keymap configuration and raises `keycode_state_changed` with the parsed keycodes.

### `keycode_state_changed`

Fired in response to processing a `position_state_changed` event which is processed against the keymap configuration and raises `keycode_state_changed` events with the parsed keycode and pressed state.

### `layer_state_changed`

Fired whenever the a layer activates or deactivates. Event contains the layer index and it's boolean activation state.

### `modifiers_state_changed`

todo

<!-- Seems to be a work in progress,  I can see definitions but no usages -->

### `sensor_event`

Fired when a sensor (e.g. an EC11 encoder) reports a change (e.g. rotation). Event contains the sensor, sensor number and timestamp.

### `usb_conn_state_changed`

Fired when the USB status changes e.g. on connected/disconnected. Event contains the USB status.
