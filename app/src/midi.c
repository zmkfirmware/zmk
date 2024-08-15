/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include "zmk/midi.h"
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
#include <dt-bindings/zmk/modifiers.h>

static struct zmk_midi_report midi_report = {
    .report_id = ZMK_REPORT_ID_MIDI,
    .body = {.cin = MIDI_INVALID, .key = MIDI_INVALID, .key_value = MIDI_INVALID}};

static bool sustain_toggle_on = false;
static bool sostenuto_toggle_on = false;

void set_bitmap(uint64_t map, uint32_t bit_num, bool value) {
    // do this in a function as WRITE_BIT
    // dirties the value in bitnum
    WRITE_BIT(map, bit_num, value);
}

bool bit_is_set(uint64_t map, uint32_t bit_num) {
    // The BIT macro modifies the value, so using it outside of a function
    // can dirty the bit_num variable
    return (map & BIT(bit_num));
}

void zmk_midi_report_clear() {
    midi_report.body.cin = MIDI_INVALID;
    midi_report.body.key = MIDI_INVALID;
    midi_report.body.key_value = MIDI_INVALID;
}

int zmk_midi_key_press(const zmk_midi_key_t key) {
    LOG_INF("zmk_midi_key_press received: 0x%04x aka %d", key, key);

    switch (key) {
    case MIDI_MIN_NOTE ... MIDI_MAX_NOTE:
        // and write and updated report
        zmk_midi_report_clear();
        midi_report.body.cin = ZMK_MIDI_CIN_NOTE_ON;
        midi_report.body.key = key;
        midi_report.body.key_value = ZMK_MIDI_ON_VELOCITY;
        break;
    case MIDI_MIN_CONTROL ... MIDI_MAX_CONTROL:
        zmk_midi_key_t control_key_transformed = (uint8_t)key;
        if (SUSTAIN == key) {
            if (!sustain_toggle_on) {
                // we set the toggle on in the release
                // since there will be 2 releases before we want
                // to turn off the toggle
                // dont set the toggle on here!
                zmk_midi_report_clear();
                midi_report.body.cin = ZMK_MIDI_CIN_CONTROL_CHANGE;
                midi_report.body.key = control_key_transformed;
                midi_report.body.key_value = ZMK_MIDI_TOGGLE_ON;
            } else {
                zmk_midi_report_clear();
                return -EINPROGRESS;
            }
        } else if (SOSTENUTO == key) {
            if (!sostenuto_toggle_on) {
                // we set the toggle on in the release
                // since there will be 2 releases before we want
                // to turn off the toggle
                // dont set the toggle on here!
                zmk_midi_report_clear();
                midi_report.body.cin = ZMK_MIDI_CIN_CONTROL_CHANGE;
                midi_report.body.key = control_key_transformed;
                midi_report.body.key_value = ZMK_MIDI_TOGGLE_ON;
            } else {
                zmk_midi_report_clear();
                return -EINPROGRESS;
            }
        } else {
            // not implemented
            zmk_midi_report_clear();
            LOG_INF("midi control handling not implemented");
        }
        return 0;
        break;
    default:
        LOG_ERR("Unsupported midi key %d", key);
        return -EINVAL;
        break;
    }

    return 0;
}

int zmk_midi_key_release(const zmk_midi_key_t key) {
    LOG_INF("zmk_midi_key_release received: 0x%04x aka %d", key, key);

    switch (key) {
    case MIDI_MIN_NOTE ... MIDI_MAX_NOTE:
        // write an updated report
        zmk_midi_report_clear();
        midi_report.body.cin = ZMK_MIDI_CIN_NOTE_OFF;
        midi_report.body.key = key;
        midi_report.body.key_value = ZMK_MIDI_OFF_VELOCITY;
        return 0;
        break;
    case MIDI_MIN_CONTROL ... MIDI_MAX_CONTROL:
        zmk_midi_key_t control_key_transformed = (uint8_t)key;
        if (SUSTAIN == key) {
            if (!sustain_toggle_on) {
                // the first release we see of a toggle we should ignore
                // otherwise it doesn't behave as a toggle!
                // just set the toggle variable
                sustain_toggle_on = true;
                zmk_midi_report_clear();
                return -EINPROGRESS;
            } else if (sustain_toggle_on) {
                sustain_toggle_on = false;
                zmk_midi_report_clear();
                midi_report.body.cin = ZMK_MIDI_CIN_CONTROL_CHANGE;
                midi_report.body.key = control_key_transformed;
                midi_report.body.key_value = ZMK_MIDI_TOGGLE_OFF;
            }
        } else if (SOSTENUTO == key) {
            if (!sostenuto_toggle_on) {
                // the first release we see of a toggle we should ignore
                // otherwise it doesn't behave as a toggle!
                // just set the toggle variable
                sostenuto_toggle_on = true;
                zmk_midi_report_clear();
                return -EINPROGRESS;
            } else if (sostenuto_toggle_on) {
                sostenuto_toggle_on = false;
                zmk_midi_report_clear();
                midi_report.body.cin = ZMK_MIDI_CIN_CONTROL_CHANGE;
                midi_report.body.key = control_key_transformed;
                midi_report.body.key_value = ZMK_MIDI_TOGGLE_OFF;
            }
        } else {
            // not implemented
            zmk_midi_report_clear();
            LOG_INF("midi control handling not implemented");
        }
        return 0;
        break;
    default:
        LOG_ERR("Unsupported midi key %d", key);
        return -EINVAL;
    }

    return 0;
}
void zmk_midi_clear(void) { memset(&midi_report.body, 0, sizeof(midi_report.body)); }

struct zmk_midi_report *zmk_get_midi_report(void) {
    return &midi_report;
}
