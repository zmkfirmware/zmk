// SPDX-License-Identifier: MIT
#include <zmk/encoder_bindings.h>
#if ZMK_STUDIO_ENCODERS
#include <stdio.h>
#include <string.h>
#include "behaviors/behavior_sensor_rotate_common.h"

// Store local behavior IDs so the settings survive device address changes.
struct encoder_record {
    uint32_t param1, param2;
    uint16_t behavior_id;
} __packed;
typedef struct encoder_record encoder_table[ZMK_KEYMAP_LAYERS_LEN][ZMK_KEYMAP_SENSORS_LEN][2];
static encoder_table active, saved;
static struct k_spinlock table_lock;
static struct behavior_sensor_rotate_data rotations;

static int stock_direction(uint8_t layer, uint8_t sensor, bool ccw,
                           struct zmk_behavior_binding *result, int *tap_ms) {
    if (layer >= ZMK_KEYMAP_LAYERS_LEN || sensor >= ZMK_KEYMAP_SENSORS_LEN)
        return -EINVAL;
    const struct zmk_behavior_binding *stock = zmk_encoder_stock(layer, sensor);
    *result = (struct zmk_behavior_binding){0};
    *tap_ms = 5;
    if (!stock->behavior_dev)
        return 0;
    const struct device *dev = zmk_behavior_get_binding(stock->behavior_dev);
    if (!dev || ((const struct behavior_driver_api *)dev->api)->sensor_binding_process !=
                    zmk_behavior_sensor_rotate_common_process)
        return -ENOTSUP;
    const struct behavior_sensor_rotate_config *cfg = dev->config;
    *result = ccw ? cfg->ccw_binding : cfg->cw_binding;
    if (cfg->override_params)
        result->param1 = ccw ? stock->param2 : stock->param1;
    *tap_ms = cfg->tap_ms;
    return 0;
}

static int direction(uint8_t layer, uint8_t sensor, bool ccw, struct zmk_behavior_binding *binding,
                     bool *overridden, int *tap_ms) {
    int rc = stock_direction(layer, sensor, ccw, binding, tap_ms);
    if (rc < 0)
        return rc;
    k_spinlock_key_t lock = k_spin_lock(&table_lock);
    struct encoder_record rec = active[layer][sensor][ccw];
    k_spin_unlock(&table_lock, lock);
    *overridden = rec.behavior_id != 0;
    if (*overridden) {
        binding->behavior_dev = zmk_behavior_find_behavior_name_from_local_id(rec.behavior_id);
        binding->param1 = rec.param1;
        binding->param2 = rec.param2;
    }
    return 0;
}

int zmk_encoder_get(uint8_t layer, uint8_t sensor, bool ccw, struct zmk_behavior_binding *binding,
                    bool *overridden) {
    int tap_ms;
    return direction(layer, sensor, ccw, binding, overridden, &tap_ms);
}

int zmk_encoder_set(uint8_t layer, uint8_t sensor, bool ccw,
                    const struct zmk_behavior_binding *binding) {
    struct zmk_behavior_binding stock;
    int tap_ms;
    int rc = stock_direction(layer, sensor, ccw, &stock, &tap_ms);
    if (rc < 0)
        return rc;
    struct encoder_record rec = {0};
    if (binding) {
        const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
        if (!zmk_behavior_accepts_key(dev) || zmk_behavior_validate_binding(binding) < 0)
            return -EINVAL;
        rec.behavior_id = zmk_behavior_get_local_id(binding->behavior_dev);
        if (!rec.behavior_id || rec.behavior_id == UINT16_MAX)
            return -ENODEV;
        rec.param1 = binding->param1;
        rec.param2 = binding->param2;
    }
    k_spinlock_key_t lock = k_spin_lock(&table_lock);
    active[layer][sensor][ccw] = rec;
    k_spin_unlock(&table_lock, lock);
    return 0;
}

int zmk_encoder_process(struct zmk_behavior_binding_event event, uint8_t sensor,
                        const struct zmk_sensor_channel_data *channel,
                        enum behavior_sensor_binding_process_mode mode) {
    struct zmk_behavior_binding binding;
    bool overridden;
    int tap_ms;
    int ticks =
        zmk_rotate_ticks(&rotations, event, zmk_sensors_get_config_at_index(sensor), channel);
    int rc = direction(event.layer, sensor, ticks < 0, &binding, &overridden, &tap_ms);
    if (rc < 0)
        return rc;
    if (!ticks || !binding.behavior_dev || mode != BEHAVIOR_SENSOR_BINDING_PROCESS_MODE_TRIGGER)
        return ZMK_BEHAVIOR_TRANSPARENT;
    return zmk_rotate_queue(event, binding, abs(ticks), tap_ms);
}

bool zmk_encoder_dirty(void) { return memcmp(active, saved, sizeof(saved)) != 0; }

int zmk_encoder_save(void) {
    for (int l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
        for (int s = 0; s < ZMK_KEYMAP_SENSORS_LEN; s++) {
            for (int d = 0; d < 2; d++) {
                struct encoder_record rec = active[l][s][d];
                if (!memcmp(&rec, &saved[l][s][d], sizeof(rec)))
                    continue;
                char path[32];
                snprintf(path, sizeof(path), "keymap/enc1/%u", (l << 16) | (s << 1) | d);
                int rc = rec.behavior_id ? settings_save_one(path, &rec, sizeof(rec))
                                         : settings_delete(path);
                if (rc < 0)
                    return rc;
                saved[l][s][d] = rec;
            }
        }
    }
    return 0;
}

void zmk_encoder_discard(void) {
    k_spinlock_key_t lock = k_spin_lock(&table_lock);
    memcpy(active, saved, sizeof(active));
    k_spin_unlock(&table_lock, lock);
}

int zmk_encoder_reset(void) {
    k_spinlock_key_t lock = k_spin_lock(&table_lock);
    memset(active, 0, sizeof(active));
    k_spin_unlock(&table_lock, lock);
    return zmk_encoder_save();
}

int zmk_encoder_load(const char *key, size_t len, settings_read_cb read_cb, void *arg) {
    char *end;
    unsigned long index = strtoul(key, &end, 10);
    unsigned long layer = index >> 16, sensor = (index & 0xffff) >> 1;
    if (end == key || *end || layer >= ZMK_KEYMAP_LAYERS_LEN || sensor >= ZMK_KEYMAP_SENSORS_LEN ||
        len != sizeof(struct encoder_record))
        return -EINVAL;
    struct encoder_record rec;
    if (read_cb(arg, &rec, len) != len)
        return -EIO;
    k_spinlock_key_t lock = k_spin_lock(&table_lock);
    active[layer][sensor][index & 1] = saved[layer][sensor][index & 1] = rec;
    k_spin_unlock(&table_lock, lock);
    return 0;
}
#endif
