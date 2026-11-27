// SPDX-License-Identifier: MIT
#pragma once

#include <drivers/behavior.h>
#include <zephyr/settings/settings.h>
#include <zmk/keymap.h>
#include <zmk/sensors.h>

#define ZMK_STUDIO_ENCODERS (IS_ENABLED(CONFIG_ZMK_STUDIO) && ZMK_KEYMAP_HAS_SENSORS)

#if ZMK_STUDIO_ENCODERS
const struct zmk_behavior_binding *zmk_encoder_stock(uint8_t layer, uint8_t sensor);
int zmk_encoder_get(uint8_t layer, uint8_t sensor, bool ccw, struct zmk_behavior_binding *binding,
                    bool *overridden);
int zmk_encoder_set(uint8_t layer, uint8_t sensor, bool ccw,
                    const struct zmk_behavior_binding *binding);
int zmk_encoder_process(struct zmk_behavior_binding_event event, uint8_t sensor,
                        const struct zmk_sensor_channel_data *channel,
                        enum behavior_sensor_binding_process_mode mode);
bool zmk_encoder_dirty(void);
int zmk_encoder_save(void);
void zmk_encoder_discard(void);
int zmk_encoder_reset(void);
int zmk_encoder_load(const char *key, size_t len, settings_read_cb read_cb, void *arg);
#endif
