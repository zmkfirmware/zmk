/*
 * Copyright (c) 2022 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_custom_lock

#include <device.h>
#include <settings/settings.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define ZMK_BHV_LOCK_KEY_MAX_HELD 10
#define ZMK_BHV_LOCK_KEY_POSITION_FREE UINT32_MAX

struct behavior_custom_lock_key_config {
    const struct device *dev;
    char *unlocked_behavior_dev;
    char *locked_behavior_dev;
};

struct behavior_custom_lock_var_data {
    bool active;
};

struct active_lock_key {
    int layer;
    uint32_t position;
    struct zmk_behavior_binding binding;
};

struct active_lock_key active_lock_keys[ZMK_BHV_LOCK_KEY_MAX_HELD] = {};

static struct active_lock_key* find_lock_key(struct zmk_behavior_binding_event *event) {
    for (int i = 0; i < ZMK_BHV_LOCK_KEY_MAX_HELD; i++) {
        if (active_lock_keys[i].position == event->position && active_lock_keys[i].layer == event->layer) {
            return &active_lock_keys[i];
        }
    }
    return NULL;
}

static int new_lock_key(struct zmk_behavior_binding_event *event, struct zmk_behavior_binding binding) {
    for (int i = 0; i < ZMK_BHV_LOCK_KEY_MAX_HELD; i++) {
        struct active_lock_key *const ref_locks = &active_lock_keys[i];
        if (ref_locks->position == ZMK_BHV_LOCK_KEY_POSITION_FREE) {
            ref_locks->position = event->position;
            ref_locks->layer = event->layer;
            ref_locks->binding = binding;
            return 0;
        }
    }
    return -ENOMEM;
}

static void clear_lock_key(struct active_lock_key *lock_key) {
    lock_key->position = ZMK_BHV_LOCK_KEY_POSITION_FREE;
}

#if IS_ENABLED(CONFIG_SETTINGS)

static void lock_save_state(const char* name, bool active) {
    char settings_name[30];
    sprintf(settings_name, "bhv/lock/%s", name);
    settings_save_one(settings_name, &active, sizeof(bool));
}

static void lock_delete_state(const char* name) {
    char settings_name[30];
    sprintf(settings_name, "bhv/lock/%s", name);
    settings_delete(settings_name);
}

static int lock_settings_load(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const struct device* dev = device_get_binding(name);

    if (dev == NULL) {
        LOG_WRN("Unknown lock device from settings %s - purging from settings", name);
        lock_delete_state(name);
        return -1;
    }

    struct behavior_custom_lock_var_data* data = dev->data;
    int rc;

    if (len != sizeof(bool)) {
        LOG_DBG("something is of with size %d", len);
        return -EINVAL;
    }

    rc = read_cb(cb_arg, &data->active, sizeof(bool));
    if (rc >= 0) {
        return 0;
    }

    return rc;
}

struct settings_handler lock_settings_conf = {.name = "bhv/lock", .h_set = lock_settings_load};

#endif

static int behavior_lock_init(const struct device *_arg) {

#if IS_ENABLED(CONFIG_SETTINGS)
    settings_subsys_init();

    int err = settings_register(&lock_settings_conf);
    if (err) {
        LOG_ERR("Failed to register the ext_power settings handler (err %d)", err);
        return err;
    }

    settings_load_subtree("bhv/lock");
#endif

    return 0;
}


static int behavior_lock_key_init(const struct device *dev) {
    return 0;
};

static int behavior_lock_var_init(const struct device *dev) {
    for (int i = 0; i < ZMK_BHV_LOCK_KEY_MAX_HELD; i++) {
        active_lock_keys[i].position = ZMK_BHV_LOCK_KEY_POSITION_FREE;
    }

    return 0; 
};

static int on_keymap_key_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_custom_lock_key_config *config = dev->config;

    const struct device *parent = config->dev;
    const bool active = ((struct behavior_custom_lock_var_data*)parent->data)->active;

    struct zmk_behavior_binding new_binding = {0};
    if (active) {
        new_binding.behavior_dev = config->locked_behavior_dev;
        new_binding.param1 = binding->param2;
    } else {
        new_binding.behavior_dev = config->unlocked_behavior_dev;
        new_binding.param1 = binding->param1;
    }

    int res = new_lock_key(&event, new_binding);
    if (res == -ENOMEM) {
        LOG_WRN("Couldn't find space to store current lock press. Ignoring key");
        return ZMK_BEHAVIOR_OPAQUE;
    }

    return behavior_keymap_binding_pressed(&new_binding, event);
}

static int on_keymap_key_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {

    struct active_lock_key *cur_key = find_lock_key(&event);

    if (cur_key == NULL) {
        LOG_WRN("Binding for layer: %d | position: %d not found. Not sure how to proceed", event.layer, event.position);
    } else {
        struct zmk_behavior_binding binding = cur_key->binding;
        clear_lock_key(cur_key);
        return behavior_keymap_binding_released(&binding, event);
    }
    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_var_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    struct behavior_custom_lock_var_data* data = dev->data;

    data->active = !data->active;
    lock_save_state(dev->name, data->active);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_var_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_lock_key_driver_api = {
    .binding_pressed = on_keymap_key_binding_pressed,
    .binding_released = on_keymap_key_binding_released
};

static const struct behavior_driver_api behavior_lock_var_driver_api = {
    .binding_pressed = on_keymap_var_binding_pressed,
    .binding_released = on_keymap_var_binding_released
};

#define CL_CHILD(id) \
    static struct behavior_custom_lock_key_config behavior_lock_key_config_##id = { \
        .dev = DEVICE_DT_GET(DT_PARENT(id)), \
        .unlocked_behavior_dev = DT_LABEL(DT_PHANDLE_BY_IDX(id, bindings, 0)), \
        .locked_behavior_dev = DT_LABEL(DT_PHANDLE_BY_IDX(id, bindings, 1)), \
    }; \
    DEVICE_DT_DEFINE(id, &behavior_lock_key_init, NULL, NULL, &behavior_lock_key_config_##id, \
           APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_lock_key_driver_api);


#define CL_INST(id) \
    DT_INST_FOREACH_CHILD(id, CL_CHILD) \
    static struct behavior_custom_lock_var_data behavior_lock_var_data_##id = { .active = false }; \
        \
    DEVICE_DT_INST_DEFINE(id, &behavior_lock_var_init, NULL, &behavior_lock_var_data_##id, \
            NULL, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_lock_var_driver_api);


DT_INST_FOREACH_STATUS_OKAY(CL_INST)

SYS_INIT(behavior_lock_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
