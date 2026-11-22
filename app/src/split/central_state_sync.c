/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/split/central.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
#include <zmk/hid_indicators.h>
#include <zmk/events/hid_indicators_changed.h>
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define CMD_BIT(_type) BIT((_type) - ZMK_SPLIT_TRANSPORT_STATE_SYNC_CMD_TYPE_FIRST)

struct source_state {
    // Commands that may differ from what the peripheral has.
    atomic_t dirty;
    // Commands with a valid entry in last_sent.
    atomic_t sent;
    struct zmk_split_transport_central_command last_sent[ZMK_SPLIT_TRANSPORT_STATE_SYNC_CMD_COUNT];
};

static struct source_state sources[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];

static void send_work_cb(struct k_work *work);

static K_WORK_DELAYABLE_DEFINE(send_work, send_work_cb);

static void schedule_send(void) {
    // Does nothing while already scheduled, so changes within the window are coalesced.
    k_work_schedule(&send_work, K_MSEC(CONFIG_ZMK_SPLIT_PERIPHERAL_CENTRAL_STATE_SYNC_COALESCE_MS));
}

static void mark_dirty(atomic_val_t cmds) {
    for (size_t i = 0; i < ARRAY_SIZE(sources); i++) {
        atomic_or(&sources[i].dirty, cmds);
    }

    schedule_send();
}

void zmk_split_central_state_sync_resync(uint8_t source) {
    if (source >= ARRAY_SIZE(sources)) {
        return;
    }

    atomic_clear(&sources[source].sent);
    // Types without an enabled domain are dropped by build_cmd() when sending.
    atomic_or(&sources[source].dirty, BIT_MASK(ZMK_SPLIT_TRANSPORT_STATE_SYNC_CMD_COUNT));
    schedule_send();
}

static int build_cmd(struct zmk_split_transport_central_command *cmd) {
    switch (cmd->type) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    case ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_HID_INDICATORS:
        cmd->data.set_hid_indicators.indicators = zmk_hid_indicators_get_current_profile();
        LOG_DBG("hid indicators %x", cmd->data.set_hid_indicators.indicators);
        return 0;
#endif // IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    default:
        return -ENOTSUP;
    }
}

static void send_work_cb(struct k_work *work) {
    uint8_t source_ids[ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT];
    int count = zmk_split_central_get_available_source_ids(source_ids);
    if (count < 0) {
        return;
    }

    bool retry = false;

    for (int i = 0; i < count; i++) {
        uint8_t source = source_ids[i];
        struct source_state *state = &sources[source];

        for (int bit = 0; bit < ZMK_SPLIT_TRANSPORT_STATE_SYNC_CMD_COUNT; bit++) {
            // Cleared up front, so a change raised while this runs marks it dirty again.
            if (!atomic_test_and_clear_bit(&state->dirty, bit)) {
                continue;
            }

            struct zmk_split_transport_central_command cmd = {
                .type = ZMK_SPLIT_TRANSPORT_STATE_SYNC_CMD_TYPE_FIRST + bit,
            };
            if (build_cmd(&cmd) < 0) {
                continue;
            }

            if (atomic_test_bit(&state->sent, bit) &&
                memcmp(&cmd, &state->last_sent[bit], sizeof(cmd)) == 0) {
                LOG_DBG("central state: type %d unchanged for %d, skipped", cmd.type, source);
                continue;
            }

            // Recorded before sending, so a resync while sending clears it again.
            state->last_sent[bit] = cmd;
            atomic_set_bit(&state->sent, bit);

            int err = zmk_split_central_send_command(source, cmd);
            if (err < 0) {
                LOG_WRN("central state: send type %d to %d failed (%d)", cmd.type, source, err);
                atomic_clear_bit(&state->sent, bit);
                atomic_set_bit(&state->dirty, bit);
                // The wired TX buffer is full; try again after it drains.
                retry |= (err == -ENOSPC);
                continue;
            }

            LOG_DBG("central state: send type %d to %d", cmd.type, source);
        }
    }

    if (retry) {
        schedule_send();
    }
}

static int state_sync_listener(const zmk_event_t *eh) {
    atomic_val_t cmds = 0;

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
    if (as_zmk_hid_indicators_changed(eh)) {
        cmds |= CMD_BIT(ZMK_SPLIT_TRANSPORT_CENTRAL_CMD_TYPE_SET_HID_INDICATORS);
    }
#endif

    if (cmds) {
        mark_dirty(cmds);
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(split_central_state_sync, state_sync_listener);

#if IS_ENABLED(CONFIG_ZMK_SPLIT_PERIPHERAL_HID_INDICATORS)
ZMK_SUBSCRIPTION(split_central_state_sync, zmk_hid_indicators_changed);
#endif
