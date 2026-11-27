/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <zmk/led_matrix.h>

#define NUMPADV2_DIAG_SETTINGS_ROOT "numpadv2_diag"
#define NUMPADV2_DIAG_SETTINGS_KEY NUMPADV2_DIAG_SETTINGS_ROOT "/flash_marker"
#define NUMPADV2_DIAG_MARKER_MAGIC UINT32_C(0x4E503244)
#define NUMPADV2_DIAG_DISPLAY_DELAY K_SECONDS(5)

enum numpadv2_diag_marker_state {
    NUMPADV2_DIAG_MARKER_ABSENT = 0,
    NUMPADV2_DIAG_MARKER_VALID,
    NUMPADV2_DIAG_MARKER_CORRUPT,
};

enum numpadv2_diag_color {
    NUMPADV2_DIAG_RED_HUE_INDEX = 0,
    NUMPADV2_DIAG_YELLOW_HUE_INDEX = 2,
    NUMPADV2_DIAG_GREEN_HUE_INDEX = 4,
};

struct numpadv2_diag_marker {
    uint32_t magic;
    uint32_t magic_inverse;
};

static enum numpadv2_diag_marker_state marker_state = NUMPADV2_DIAG_MARKER_ABSENT;

static void numpadv2_diag_show_color(uint8_t hue_index) {
    /* numpadv2 profile 0 is the static LM_PALETTE profile. */
    (void)zmk_led_matrix_set_profile(0);
    (void)zmk_led_matrix_set_hue_index(hue_index);
    (void)zmk_led_matrix_set_brt(100);
}

static void numpadv2_diag_work_handler(struct k_work *work) {
    ARG_UNUSED(work);

    if (marker_state == NUMPADV2_DIAG_MARKER_VALID) {
        numpadv2_diag_show_color(NUMPADV2_DIAG_GREEN_HUE_INDEX);
        return;
    }

    if (marker_state == NUMPADV2_DIAG_MARKER_CORRUPT) {
        numpadv2_diag_show_color(NUMPADV2_DIAG_RED_HUE_INDEX);
        return;
    }

    const struct numpadv2_diag_marker marker = {
        .magic = NUMPADV2_DIAG_MARKER_MAGIC,
        .magic_inverse = ~NUMPADV2_DIAG_MARKER_MAGIC,
    };
    int err = settings_save_one(NUMPADV2_DIAG_SETTINGS_KEY, &marker, sizeof(marker));

    numpadv2_diag_show_color(err == 0 ? NUMPADV2_DIAG_YELLOW_HUE_INDEX
                                      : NUMPADV2_DIAG_RED_HUE_INDEX);
}

K_WORK_DELAYABLE_DEFINE(numpadv2_diag_work, numpadv2_diag_work_handler);

static int numpadv2_diag_settings_set(const char *name, size_t len,
                                      settings_read_cb read_cb, void *cb_arg) {
    const char *next;

    if (!settings_name_steq(name, "flash_marker", &next) || next != NULL) {
        return -ENOENT;
    }

    if (len != sizeof(struct numpadv2_diag_marker)) {
        marker_state = NUMPADV2_DIAG_MARKER_CORRUPT;
        return 0;
    }

    struct numpadv2_diag_marker marker;
    int err = read_cb(cb_arg, &marker, sizeof(marker));
    if (err < 0 || marker.magic != NUMPADV2_DIAG_MARKER_MAGIC ||
        marker.magic_inverse != ~NUMPADV2_DIAG_MARKER_MAGIC) {
        marker_state = NUMPADV2_DIAG_MARKER_CORRUPT;
        return 0;
    }

    marker_state = NUMPADV2_DIAG_MARKER_VALID;
    return 0;
}

static int numpadv2_diag_settings_commit(void) {
    int err = k_work_reschedule(&numpadv2_diag_work, NUMPADV2_DIAG_DISPLAY_DELAY);

    return err < 0 ? err : 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(numpadv2_diag, NUMPADV2_DIAG_SETTINGS_ROOT, NULL,
                               numpadv2_diag_settings_set, numpadv2_diag_settings_commit, NULL);
