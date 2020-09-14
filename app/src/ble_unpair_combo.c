/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <device.h>
#include <init.h>

#define DT_DRV_COMPAT zmk_bt_unpair_combo

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/ble.h>
#include <zmk/event-manager.h>
#include <zmk/events/position-state-changed.h>

static u8_t combo_state;

const u32_t key_positions[] = DT_INST_PROP(0, key_positions);
#define KP_LEN DT_INST_PROP_LEN(0, key_positions)

int index_for_key_position(u32_t kp) {
    for (int i = 0; i < KP_LEN; i++) {
        if (key_positions[i] == kp) {
            return i;
        }
    }

    return -1;
}

int unpair_combo_listener(const struct zmk_event_header *eh) {
    if (is_position_state_changed(eh)) {
        const struct position_state_changed *psc = cast_position_state_changed(eh);

        int kp_index = index_for_key_position(psc->position);
        if (kp_index < 0) {
            return 0;
        }

        WRITE_BIT(combo_state, kp_index, psc->state);
    }

    return 0;
};

void unpair_combo_work_handler(struct k_work *work) {
    for (int i = 0; i < KP_LEN; i++) {
        if (!(combo_state & BIT(i))) {
            LOG_DBG("Key position %d not held, skipping unpair combo", key_positions[i]);
            return;
        }
    }

    zmk_ble_unpair_all();
};

struct k_delayed_work unpair_combo_work;

int zmk_ble_unpair_combo_init(struct device *_unused) {
    k_delayed_work_init(&unpair_combo_work, unpair_combo_work_handler);
    k_delayed_work_submit(&unpair_combo_work, K_SECONDS(2));

    return 0;
};

ZMK_LISTENER(zmk_ble_unpair_combo, unpair_combo_listener);
ZMK_SUBSCRIPTION(zmk_ble_unpair_combo, position_state_changed);

SYS_INIT(zmk_ble_unpair_combo_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
