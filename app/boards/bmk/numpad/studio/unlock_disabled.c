/* SPDX-License-Identifier: MIT */

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

/* Without this fallback, an unavailable &studio_unlock falls through to Base
 * and FN+0 sends a zero. Preserve the former &none behavior in diagnostics.
 */
static int ignore_binding(struct zmk_behavior_binding *binding,
                          struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api unlock_disabled_api = {
    .binding_pressed = ignore_binding,
    .binding_released = ignore_binding,
};

BEHAVIOR_DT_DEFINE(DT_NODELABEL(studio_unlock), NULL, NULL, NULL, NULL, POST_KERNEL,
                   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &unlock_disabled_api);
