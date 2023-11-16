/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_temp_layer

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>

struct k_work_delayable layer_disable_works[ZMK_KEYMAP_LAYERS_LEN];

static void layer_disable_cb(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    int layer_index = ARRAY_INDEX(layer_disable_works, d_work);
    if (zmk_keymap_layer_active(layer_index)) {
        zmk_keymap_layer_deactivate(layer_index);
    }
}

static int tl_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                           uint32_t param2, struct zmk_input_processor_state *state) {
    if (param1 >= ZMK_KEYMAP_LAYERS_LEN) {
        LOG_ERR("Passed an invalid layer id %d", param1);
        return -EINVAL;
    }

    if (!zmk_keymap_layer_active(param1)) {
        zmk_keymap_layer_activate(param1);
    }

    k_work_reschedule(&layer_disable_works[param1], K_MSEC(param2));
    return 0;
}

static struct zmk_input_processor_driver_api tl_driver_api = {
    .handle_event = tl_handle_event,
};

static int tl_init(const struct device *dev) {
    for (int i = 0; i < ZMK_KEYMAP_LAYERS_LEN; i++) {
        k_work_init_delayable(&layer_disable_works[i], layer_disable_cb);
    }

    return 0;
}

#define TL_INST(n)                                                                                 \
    DEVICE_DT_INST_DEFINE(n, &tl_init, NULL, NULL, NULL, POST_KERNEL,                              \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &tl_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TL_INST)