/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk_studio, CONFIG_ZMK_STUDIO_LOG_LEVEL);

#include <drivers/behavior.h>
#include <zmk/combos.h>
#include <zmk/studio/rpc.h>

#include <pb_encode.h>

ZMK_RPC_SUBSYSTEM(combos)

#define COMBOS_RESPONSE(type, ...) ZMK_RPC_RESPONSE(combos, type, __VA_ARGS__)
#define COMBOS_NOTIFICATION(type, ...) ZMK_RPC_NOTIFICATION(combos, type, __VA_ARGS__)

static bool encode_combo_positions(pb_ostream_t *stream, const pb_field_t *field,
                                   void *const *arg) {
    const struct zmk_combo_runtime *rc = (const struct zmk_combo_runtime *)*arg;

    if (!rc) {
        return false;
    }

    for (int i = 0; i < rc->combo.key_position_len; i++) {
        if (!pb_encode_tag_for_field(stream, field)) {
            LOG_WRN("Failed to encode tag");
            return false;
        }

        pb_encode_varint(stream, rc->combo.key_positions[i]);
    }

    return true;
}

static bool encode_combos(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    const struct zmk_combo_runtime *list;
    int count = zmk_combo_runtime_get_combos(&list);

    for (size_t i = 0; i < count; i++) {
        const struct zmk_combo_runtime *rc = &list[i];

        if (!pb_encode_tag_for_field(stream, field)) {
            LOG_WRN("Failed to encode tag");
            return false;
        }

        zmk_combos_Combo combo = zmk_combos_Combo_init_zero;

        combo.has_binding = true;
        combo.id = rc->id;

        combo.positions.funcs.encode = encode_combo_positions;
        combo.positions.arg = (void *)rc;

        combo.binding.behavior_id = zmk_behavior_get_local_id(rc->combo.behavior.behavior_dev);
        combo.binding.param1 = rc->combo.behavior.param1;
        combo.binding.param2 = rc->combo.behavior.param2;

        combo.timeout_ms = rc->combo.timeout_ms;
        if (rc->combo.require_prior_idle_ms > 0) {
            combo.require_prior_idle_ms = rc->combo.require_prior_idle_ms;
        }

        combo.slow_release = rc->combo.slow_release;
        combo.layer_mask = rc->combo.layer_mask;

        if (!pb_encode_submessage(stream, &zmk_combos_Combo_msg, &combo)) {
            LOG_WRN("Failed to encode combo submessage");
            return false;
        }
    }

    return true;
}

static zmk_studio_Response get_combos(const zmk_studio_Request *req) {
    LOG_DBG("");

    zmk_combos_GetCombosResponse resp = zmk_combos_GetCombosResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_GetCombosResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
#else

    resp.which_result = zmk_combos_GetCombosResponse_ok_tag;
    resp.result.ok.free_combos = zmk_combo_runtime_get_free_combos();
    resp.result.ok.combos.funcs.encode = encode_combos;

#endif

    return COMBOS_RESPONSE(get_combos, resp);
}

static zmk_studio_Response delete_combo(const zmk_studio_Request *req) {
    const zmk_combos_ComboIdRequest *del_req = &req->subsystem.combos.request_type.delete_combo;

    LOG_DBG("%d", del_req->id);
    zmk_combos_DeleteComboResponse resp = zmk_combos_DeleteComboResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_DeleteComboResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
    return COMBOS_RESPONSE(delete_combo, resp);
#else

    int ret = zmk_combo_runtime_remove_combo(del_req->id);

    if (ret < 0) {
        LOG_ERR("Failed to delete combo %d", ret);
        resp.which_result = zmk_combos_DeleteComboResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_FOUND;
            break;
        default:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
            break;
        }
    } else {
        resp.which_result = zmk_combos_DeleteComboResponse_ok_tag;
        resp.result.ok = true;
    }

    return COMBOS_RESPONSE(delete_combo, resp);
#endif
}

static zmk_studio_Response add_combo(const zmk_studio_Request *req) {
    const zmk_combos_AddComboRequest *add_req = &req->subsystem.combos.request_type.add_combo;

    zmk_combos_AddComboResponse resp = zmk_combos_AddComboResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_AddComboResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
    return COMBOS_RESPONSE(add_combo, resp);
#else

    LOG_DBG("behavior: %d, param1: %d, param2: %d", add_req->binding.behavior_id,
            add_req->binding.param1, add_req->binding.param2);

    const char *behavior_dev =
        zmk_behavior_find_behavior_name_from_local_id(add_req->binding.behavior_id);

    if (!behavior_dev) {
        resp.which_result = zmk_combos_AddComboResponse_err_tag;
        resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_INVALID;
        return COMBOS_RESPONSE(add_combo, resp);
    }

    if (add_req->positions_count < 2 || add_req->positions_count > MAX_COMBO_KEYS) {
        resp.which_result = zmk_combos_AddComboResponse_err_tag;
        resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_INVALID;
        return COMBOS_RESPONSE(add_combo, resp);
    }

    struct combo_cfg combo_cfg = {
        .behavior =
            (struct zmk_behavior_binding){
                .local_id = add_req->binding.behavior_id,
                .behavior_dev = behavior_dev,
                .param1 = add_req->binding.param1,
                .param2 = add_req->binding.param2,
            },
        .timeout_ms = add_req->timeout_ms,
        .require_prior_idle_ms = add_req->require_prior_idle_ms,
        .slow_release = add_req->slow_release,
        .layer_mask = add_req->layer_mask,
    };

    for (int i = 0; i < add_req->positions_count; i++) {
        combo_cfg.key_positions[i] = add_req->positions[i];
    }

    combo_cfg.key_position_len = add_req->positions_count;

    int ret = zmk_behavior_validate_binding(&combo_cfg.behavior);
    if (ret < 0) {
        LOG_ERR("Invalid binding (%d)", ret);
        resp.which_result = zmk_combos_AddComboResponse_err_tag;
        resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_INVALID;
        return COMBOS_RESPONSE(add_combo, resp);
    }

    ret = zmk_combo_runtime_add_combo(&combo_cfg);

    if (ret < 0) {
        LOG_ERR("Failed to create the combo %d", ret);
        resp.which_result = zmk_combos_AddComboResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_FOUND;
            break;
        default:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
            break;
        }
    } else {
        resp.which_result = zmk_combos_AddComboResponse_ok_new_id_tag;
        resp.result.ok_new_id = ret;
    }

    return COMBOS_RESPONSE(add_combo, resp);
#endif
}

static zmk_studio_Response set_combo_binding(const zmk_studio_Request *req) {
    const zmk_combos_ComboBindingRequest *cb_req =
        &req->subsystem.combos.request_type.set_combo_binding;

    zmk_combos_ComboChangeResponse resp = zmk_combos_ComboChangeResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
    return COMBOS_RESPONSE(set_combo_binding, resp);
#else

    if (!cb_req->has_binding) {
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
        return COMBOS_RESPONSE(set_combo_binding, resp);
    }

    LOG_DBG("id: %d, behavior: %d, param1: %d, param2: %d", cb_req->id, cb_req->binding.behavior_id,
            cb_req->binding.param1, cb_req->binding.param2);

    const char *behavior_dev =
        zmk_behavior_find_behavior_name_from_local_id(cb_req->binding.behavior_id);

    if (!behavior_dev) {
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_INVALID;
        return COMBOS_RESPONSE(set_combo_binding, resp);
    }

    struct zmk_behavior_binding binding = (struct zmk_behavior_binding){
        .local_id = cb_req->binding.behavior_id,
        .behavior_dev = behavior_dev,
        .param1 = cb_req->binding.param1,
        .param2 = cb_req->binding.param2,
    };

    int ret = zmk_behavior_validate_binding(&binding);
    if (ret < 0) {
        LOG_ERR("Invalid binding (%d)", ret);
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_INVALID;
        return COMBOS_RESPONSE(set_combo_binding, resp);
    }

    ret = zmk_combo_runtime_set_combo_binding(cb_req->id, &binding);
    if (ret < 0) {
        LOG_ERR("Failed to set combo binding %d", ret);
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_FOUND;
            break;
        default:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
            break;
        }
    } else {
        resp.which_result = zmk_combos_ComboChangeResponse_ok_tag;
        resp.result.ok = true;
    }

    return COMBOS_RESPONSE(set_combo_binding, resp);
#endif
}

static zmk_studio_Response set_combo_position_state(const zmk_studio_Request *req) {
    const zmk_combos_ComboPositionRequest *cp_req =
        &req->subsystem.combos.request_type.set_combo_position_state;

    LOG_DBG("id: %d, position: %d, enabled: %s", cp_req->id, cp_req->position,
            cp_req->enabled ? "true" : "false");
    zmk_combos_ComboChangeResponse resp = zmk_combos_ComboChangeResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
    return COMBOS_RESPONSE(set_combo_position_state, resp);
#else

    int ret = cp_req->enabled
                  ? zmk_combo_runtime_add_combo_position(cp_req->id, cp_req->position)
                  : zmk_combo_runtime_remove_combo_position(cp_req->id, cp_req->position);

    if (ret < 0) {
        LOG_ERR("Failed to %s combo position %d", (cp_req->enabled ? "enable" : "disable"), ret);
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_FOUND;
            break;
        default:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
            break;
        }
    } else {
        resp.which_result = zmk_combos_ComboChangeResponse_ok_tag;
        resp.result.ok = true;
    }

    return COMBOS_RESPONSE(set_combo_position_state, resp);
#endif
}

static zmk_studio_Response clear_combo_layers(const zmk_studio_Request *req) {
    const zmk_combos_ComboIdRequest *l_req = &req->subsystem.combos.request_type.clear_combo_layers;

    LOG_DBG("%d", l_req->id);
    zmk_combos_ComboChangeResponse resp = zmk_combos_ComboChangeResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
    return COMBOS_RESPONSE(clear_combo_layers, resp);
#else

    int ret = zmk_combo_runtime_clear_combo_layers(l_req->id);
    if (ret < 0) {
        LOG_ERR("Failed to clear combo layers %d", ret);
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_FOUND;
            break;
        default:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
            break;
        }
    } else {
        resp.which_result = zmk_combos_ComboChangeResponse_ok_tag;
        resp.result.ok = true;
    }

    return COMBOS_RESPONSE(clear_combo_layers, resp);
#endif
}

static zmk_studio_Response set_combo_layer_state(const zmk_studio_Request *req) {
    const zmk_combos_ComboLayerRequest *cl_req =
        &req->subsystem.combos.request_type.set_combo_layer_state;

    LOG_DBG("id: %d, layer: %d, enabled: %s", cl_req->id, cl_req->layer,
            cl_req->enabled ? "true" : "false");
    zmk_combos_ComboChangeResponse resp = zmk_combos_ComboChangeResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
    return COMBOS_RESPONSE(set_combo_layer_state, resp);
#else

    int ret = zmk_combo_runtime_set_combo_layer(cl_req->id, cl_req->layer, cl_req->enabled);

    if (ret < 0) {
        LOG_ERR("Failed to %s combo layer %d", (cl_req->enabled ? "enable" : "disable"), ret);
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_FOUND;
            break;
        default:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
            break;
        }
    } else {
        resp.which_result = zmk_combos_ComboChangeResponse_ok_tag;
        resp.result.ok = true;
    }

    return COMBOS_RESPONSE(set_combo_layer_state, resp);
#endif
}

static zmk_studio_Response set_combo_slow_release_state(const zmk_studio_Request *req) {
    const zmk_combos_ComboSlowReleaseRequest *sr_req =
        &req->subsystem.combos.request_type.set_combo_slow_release_state;

    LOG_DBG("id: %d, enabled: %s", sr_req->id, sr_req->enabled ? "true" : "false");
    zmk_combos_ComboChangeResponse resp = zmk_combos_ComboChangeResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
    return COMBOS_RESPONSE(set_combo_slow_release_state, resp);
#else

    int ret = zmk_combo_runtime_set_combo_slow_release(sr_req->id, sr_req->enabled);

    if (ret < 0) {
        LOG_ERR("Failed to %s combo slow release %d", (sr_req->enabled ? "enable" : "disable"),
                ret);
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_FOUND;
            break;
        default:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
            break;
        }
    } else {
        resp.which_result = zmk_combos_ComboChangeResponse_ok_tag;
        resp.result.ok = true;
    }

    return COMBOS_RESPONSE(set_combo_slow_release_state, resp);
#endif
}

static zmk_studio_Response set_combo_timeout(const zmk_studio_Request *req) {
    const zmk_combos_ComboTimeoutRequest *t_req =
        &req->subsystem.combos.request_type.set_combo_timeout;

    LOG_DBG("id: %d, timeout: %d", t_req->id, t_req->timeout);
    zmk_combos_ComboChangeResponse resp = zmk_combos_ComboChangeResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
    return COMBOS_RESPONSE(set_combo_timeout, resp);
#else

    int ret = zmk_combo_runtime_set_combo_timeout(t_req->id, t_req->timeout);

    if (ret < 0) {
        LOG_ERR("Failed to set combo timeout %d", ret);
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_FOUND;
            break;
        default:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
            break;
        }
    } else {
        resp.which_result = zmk_combos_ComboChangeResponse_ok_tag;
        resp.result.ok = true;
    }

    return COMBOS_RESPONSE(set_combo_timeout, resp);
#endif
}

static zmk_studio_Response set_combo_require_prior_idle(const zmk_studio_Request *req) {
    const zmk_combos_ComboRequirePriorIdleRequest *t_req =
        &req->subsystem.combos.request_type.set_combo_require_prior_idle;

    LOG_DBG("id: %d, req prior idle: %d", t_req->id, t_req->require_prior_idle);
    zmk_combos_ComboChangeResponse resp = zmk_combos_ComboChangeResponse_init_zero;

#if !IS_ENABLED(CONFIG_ZMK_COMBOS_RUNTIME)
    resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
    resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_SUPPORTED;
    return COMBOS_RESPONSE(set_combo_require_prior_idle, resp);
#else

    int ret = zmk_combo_runtime_set_combo_prior_idle(t_req->id, t_req->require_prior_idle);

    if (ret < 0) {
        LOG_ERR("Failed to set combo req prior idle %d", ret);
        resp.which_result = zmk_combos_ComboChangeResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_NOT_FOUND;
            break;
        default:
            resp.result.err = zmk_combos_ComboErrorCode_COMBO_ERROR_CODE_GENERIC;
            break;
        }
    } else {
        resp.which_result = zmk_combos_ComboChangeResponse_ok_tag;
        resp.result.ok = true;
    }

    return COMBOS_RESPONSE(set_combo_require_prior_idle, resp);
#endif
}

static int combos_subsys_save_changes(void) { return zmk_combos_save_changes(); }

static int combos_subsys_discard_changes(void) { return zmk_combos_discard_changes(); }

static bool combos_check_unsaved_changes(void) { return zmk_combos_check_unsaved_changes(); }

static int combos_settings_reset(void) { return zmk_combos_reset_settings(); }

ZMK_RPC_SUBSYSTEM_HANDLER(combos, get_combos, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(combos, add_combo, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(combos, delete_combo, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(combos, set_combo_binding, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(combos, set_combo_position_state, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(combos, set_combo_layer_state, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(combos, clear_combo_layers, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(combos, set_combo_slow_release_state, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(combos, set_combo_timeout, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(combos, set_combo_require_prior_idle, ZMK_STUDIO_RPC_HANDLER_SECURED);

ZMK_RPC_SUBSYSTEM_PERSISTENCE(combos, .reset_settings = combos_settings_reset,
                              .check_unsaved_changes = combos_check_unsaved_changes,
                              .save_changes = combos_subsys_save_changes,
                              .discard_changes = combos_subsys_discard_changes, );

// static int event_mapper(const zmk_event_t *eh, zmk_studio_Notification *n) { return 0; }

// ZMK_RPC_EVENT_MAPPER(keymap, event_mapper);
