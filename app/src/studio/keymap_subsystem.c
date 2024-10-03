/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk_studio, CONFIG_ZMK_STUDIO_LOG_LEVEL);

#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/matrix.h>
#include <zmk/keymap.h>
#include <zmk/studio/rpc.h>
#include <zmk/physical_layouts.h>

#include <pb_encode.h>

ZMK_RPC_SUBSYSTEM(keymap)

#define KEYMAP_RESPONSE(type, ...) ZMK_RPC_RESPONSE(keymap, type, __VA_ARGS__)
#define KEYMAP_NOTIFICATION(type, ...) ZMK_RPC_NOTIFICATION(keymap, type, __VA_ARGS__)

static bool encode_layer_bindings(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    const zmk_keymap_layer_id_t layer_id = *(uint8_t *)*arg;

    for (int b = 0; b < ZMK_KEYMAP_LEN; b++) {
        const struct zmk_behavior_binding *binding =
            zmk_keymap_get_layer_binding_at_idx(layer_id, b);

        zmk_keymap_BehaviorBinding bb = zmk_keymap_BehaviorBinding_init_zero;

        if (binding && binding->behavior_dev) {
            bb.behavior_id = zmk_behavior_get_local_id(binding->behavior_dev);
            bb.param1 = binding->param1;
            bb.param2 = binding->param2;
        }

        if (!pb_encode_tag_for_field(stream, field)) {
            return false;
        }

        if (!pb_encode_submessage(stream, &zmk_keymap_BehaviorBinding_msg, &bb)) {
            return false;
        }
    }

    return true;
}

static bool encode_layer_name(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    const zmk_keymap_layer_index_t layer_idx = *(uint8_t *)*arg;

    const char *name = zmk_keymap_layer_name(layer_idx);

    if (!name) {
        return true;
    }

    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    return pb_encode_string(stream, name, strlen(name));
}

static bool encode_keymap_layers(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    for (zmk_keymap_layer_index_t l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
        zmk_keymap_layer_id_t layer_id = zmk_keymap_layer_index_to_id(l);

        if (layer_id == UINT8_MAX) {
            break;
        }

        if (!pb_encode_tag_for_field(stream, field)) {
            LOG_WRN("Failed to encode tag");
            return false;
        }

        zmk_keymap_Layer layer = zmk_keymap_Layer_init_zero;
        layer.id = layer_id;

        layer.name.funcs.encode = encode_layer_name;
        layer.name.arg = &layer_id;

        layer.bindings.funcs.encode = encode_layer_bindings;
        layer.bindings.arg = &layer_id;

        if (!pb_encode_submessage(stream, &zmk_keymap_Layer_msg, &layer)) {
            LOG_WRN("Failed to encode layer submessage");
            return false;
        }
    }

    return true;
}

zmk_studio_Response get_keymap(const zmk_studio_Request *req) {
    LOG_DBG("");
    zmk_keymap_Keymap resp = zmk_keymap_Keymap_init_zero;

    resp.layers.funcs.encode = encode_keymap_layers;

    resp.max_layer_name_length = CONFIG_ZMK_KEYMAP_LAYER_NAME_MAX_LEN;
    resp.available_layers = 0;

    for (zmk_keymap_layer_index_t index = 0; index < ZMK_KEYMAP_LAYERS_LEN; index++) {
        zmk_keymap_layer_id_t id = zmk_keymap_layer_index_to_id(index);

        if (id == UINT8_MAX) {
            resp.available_layers = ZMK_KEYMAP_LAYERS_LEN - index;
            break;
        }
    }

    return KEYMAP_RESPONSE(get_keymap, resp);
}

zmk_studio_Response set_layer_binding(const zmk_studio_Request *req) {
    LOG_DBG("");
    const zmk_keymap_SetLayerBindingRequest *set_req =
        &req->subsystem.keymap.request_type.set_layer_binding;

    zmk_behavior_local_id_t bid = set_req->binding.behavior_id;

    const char *behavior_name = zmk_behavior_find_behavior_name_from_local_id(bid);

    if (!behavior_name) {
        return KEYMAP_RESPONSE(
            set_layer_binding,
            zmk_keymap_SetLayerBindingResponse_SET_LAYER_BINDING_RESP_INVALID_BEHAVIOR);
    }

    struct zmk_behavior_binding binding = (struct zmk_behavior_binding){
        .behavior_dev = behavior_name,
        .param1 = set_req->binding.param1,
        .param2 = set_req->binding.param2,
    };

    int ret = zmk_behavior_validate_binding(&binding);
    if (ret < 0) {
        return KEYMAP_RESPONSE(
            set_layer_binding,
            zmk_keymap_SetLayerBindingResponse_SET_LAYER_BINDING_RESP_INVALID_PARAMETERS);
    }

    ret = zmk_keymap_set_layer_binding_at_idx(set_req->layer_id, set_req->key_position, binding);

    if (ret < 0) {
        LOG_WRN("Setting the binding failed with %d", ret);
        switch (ret) {
        case -EINVAL:
            return KEYMAP_RESPONSE(
                set_layer_binding,
                zmk_keymap_SetLayerBindingResponse_SET_LAYER_BINDING_RESP_INVALID_LOCATION);
        default:
            return ZMK_RPC_SIMPLE_ERR(GENERIC);
        }
    }

    raise_zmk_studio_rpc_notification((struct zmk_studio_rpc_notification){
        .notification = KEYMAP_NOTIFICATION(unsaved_changes_status_changed, true)});

    return KEYMAP_RESPONSE(set_layer_binding,
                           zmk_keymap_SetLayerBindingResponse_SET_LAYER_BINDING_RESP_OK);
}

zmk_studio_Response check_unsaved_changes(const zmk_studio_Request *req) {
    LOG_DBG("");
    int layout_changes = zmk_physical_layouts_check_unsaved_selection();
    int keymap_changes = zmk_keymap_check_unsaved_changes();

    return KEYMAP_RESPONSE(check_unsaved_changes, layout_changes > 0 || keymap_changes > 0);
}

static void map_errno_to_save_resp(int err, zmk_keymap_SaveChangesResponse *resp) {
    resp->which_result = zmk_keymap_SaveChangesResponse_err_tag;

    switch (err) {
    case -ENOTSUP:
        resp->result.err = zmk_keymap_SaveChangesErrorCode_SAVE_CHANGES_ERR_NOT_SUPPORTED;
        break;
    case -ENOSPC:
        resp->result.err = zmk_keymap_SaveChangesErrorCode_SAVE_CHANGES_ERR_NO_SPACE;
        break;
    default:
        resp->result.err = zmk_keymap_SaveChangesErrorCode_SAVE_CHANGES_ERR_GENERIC;
        break;
    }
}

zmk_studio_Response save_changes(const zmk_studio_Request *req) {
    zmk_keymap_SaveChangesResponse resp = zmk_keymap_SaveChangesResponse_init_zero;
    resp.which_result = zmk_keymap_SaveChangesResponse_ok_tag;
    resp.result.ok = true;

    LOG_DBG("");
    int ret = zmk_physical_layouts_save_selected();

    if (ret < 0) {
        LOG_WRN("Failed to save selected physical layout (%d)", ret);
        map_errno_to_save_resp(ret, &resp);
        return KEYMAP_RESPONSE(save_changes, resp);
    }

    ret = zmk_keymap_save_changes();
    if (ret < 0) {
        LOG_WRN("Failed to save keymap changes (%d)", ret);
        map_errno_to_save_resp(ret, &resp);
        return KEYMAP_RESPONSE(save_changes, resp);
    }

    raise_zmk_studio_rpc_notification((struct zmk_studio_rpc_notification){
        .notification = KEYMAP_NOTIFICATION(unsaved_changes_status_changed, false)});

    return KEYMAP_RESPONSE(save_changes, resp);
}

zmk_studio_Response discard_changes(const zmk_studio_Request *req) {
    LOG_DBG("");
    int ret = zmk_physical_layouts_revert_selected();
    if (ret < 0) {
        return ZMK_RPC_SIMPLE_ERR(GENERIC);
    }

    ret = zmk_keymap_discard_changes();
    if (ret < 0) {
        return ZMK_RPC_SIMPLE_ERR(GENERIC);
    }

    raise_zmk_studio_rpc_notification((struct zmk_studio_rpc_notification){
        .notification = KEYMAP_NOTIFICATION(unsaved_changes_status_changed, false)});

    return KEYMAP_RESPONSE(discard_changes, true);
}

static int keymap_settings_reset(void) { return zmk_keymap_reset_settings(); }

ZMK_RPC_SUBSYSTEM_SETTINGS_RESET(keymap, keymap_settings_reset);

static bool encode_layout_name(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    struct zmk_physical_layout *layout = (struct zmk_physical_layout *)*arg;

    if (!layout->display_name) {
        return true;
    }

    if (!pb_encode_tag_for_field(stream, field)) {
        LOG_WRN("Failed to encode tag");
        return false;
    }

    pb_encode_string(stream, layout->display_name, strlen(layout->display_name));

    return true;
}

static bool encode_layout_keys(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    struct zmk_physical_layout *layout = (struct zmk_physical_layout *)*arg;

    for (int kp = 0; kp < layout->keys_len; kp++) {
        const struct zmk_key_physical_attrs *layout_kp = &layout->keys[kp];

        if (!pb_encode_tag_for_field(stream, field)) {
            LOG_WRN("Failed to encode tag");
            return false;
        }

        zmk_keymap_KeyPhysicalAttrs layout_kp_msg = {
            .width = layout_kp->width,
            .height = layout_kp->height,
            .x = layout_kp->x,
            .y = layout_kp->y,
            .r = layout_kp->r,
            .rx = layout_kp->rx,
            .ry = layout_kp->ry,
        };

        if (!pb_encode_submessage(stream, &zmk_keymap_KeyPhysicalAttrs_msg, &layout_kp_msg)) {
            LOG_WRN("Failed to encode layout key position submessage");
            return false;
        }
    }
    return true;
}

static bool encode_layouts(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    struct zmk_physical_layout const *const *layouts;
    const size_t layout_count = zmk_physical_layouts_get_list(&layouts);

    for (int i = 0; i < layout_count; i++) {
        const struct zmk_physical_layout *l = layouts[i];

        if (!pb_encode_tag_for_field(stream, field)) {
            LOG_WRN("Failed to encode tag");
            return false;
        }

        zmk_keymap_PhysicalLayout layout = zmk_keymap_PhysicalLayout_init_zero;

        layout.name.funcs.encode = encode_layout_name;
        layout.name.arg = l;

        layout.keys.funcs.encode = encode_layout_keys;
        layout.keys.arg = l;

        if (!pb_encode_submessage(stream, &zmk_keymap_PhysicalLayout_msg, &layout)) {
            LOG_WRN("Failed to encode layout submessage");
            return false;
        }
    }

    return true;
}

zmk_studio_Response get_physical_layouts(const zmk_studio_Request *req) {
    LOG_DBG("");
    zmk_keymap_PhysicalLayouts resp = zmk_keymap_PhysicalLayouts_init_zero;
    resp.active_layout_index = zmk_physical_layouts_get_selected();
    resp.layouts.funcs.encode = encode_layouts;
    return KEYMAP_RESPONSE(get_physical_layouts, resp);
}

static void migrate_keymap(const uint8_t old) {
    int new = zmk_physical_layouts_get_selected();

    uint32_t new_to_old_map[ZMK_KEYMAP_LEN];
    int layout_size =
        zmk_physical_layouts_get_position_map(old, new, ZMK_KEYMAP_LEN, new_to_old_map);

    if (layout_size < 0) {
        return;
    }

    for (int l = 0; l < ZMK_KEYMAP_LAYERS_LEN; l++) {
        struct zmk_behavior_binding new_layer[ZMK_KEYMAP_LEN];

        for (int b = 0; b < layout_size; b++) {
            uint32_t old_b = new_to_old_map[b];

            if (old_b == UINT32_MAX) {
                memset(&new_layer[b], 0, sizeof(struct zmk_behavior_binding));
                continue;
            }

            const struct zmk_behavior_binding *binding =
                zmk_keymap_get_layer_binding_at_idx(l, old_b);

            if (!binding) {
                memset(&new_layer[b], 0, sizeof(struct zmk_behavior_binding));
                continue;
            }

            memcpy(&new_layer[b], binding, sizeof(struct zmk_behavior_binding));
        }

        for (int b = 0; b < layout_size; b++) {
            zmk_keymap_set_layer_binding_at_idx(l, b, new_layer[b]);
        }
    }

    // TODO: Migrate combos?
}

zmk_studio_Response set_active_physical_layout(const zmk_studio_Request *req) {
    LOG_DBG("");
    uint8_t index = (uint8_t)req->subsystem.keymap.request_type.set_active_physical_layout;
    int old = zmk_physical_layouts_get_selected();

    zmk_keymap_SetActivePhysicalLayoutResponse resp =
        zmk_keymap_SetActivePhysicalLayoutResponse_init_zero;
    resp.which_result = zmk_keymap_SetActivePhysicalLayoutResponse_ok_tag;
    resp.result.ok.layers.funcs.encode = encode_keymap_layers;

    if (old == index) {
        return KEYMAP_RESPONSE(set_active_physical_layout, resp);
    }

    int ret = zmk_physical_layouts_select(index);
    if (ret >= 0) {
        migrate_keymap(old);
    } else {
        resp.which_result = zmk_keymap_SetActivePhysicalLayoutResponse_err_tag;
        resp.result.err =
            zmk_keymap_SetActivePhysicalLayoutErrorCode_SET_ACTIVE_PHYSICAL_LAYOUT_ERR_GENERIC;
    }

    raise_zmk_studio_rpc_notification((struct zmk_studio_rpc_notification){
        .notification = KEYMAP_NOTIFICATION(unsaved_changes_status_changed, true)});

    return KEYMAP_RESPONSE(set_active_physical_layout, resp);
}

zmk_studio_Response move_layer(const zmk_studio_Request *req) {
    LOG_DBG("");
    const zmk_keymap_MoveLayerRequest *move_req = &req->subsystem.keymap.request_type.move_layer;

    zmk_keymap_MoveLayerResponse resp = zmk_keymap_MoveLayerResponse_init_zero;

    int ret = zmk_keymap_move_layer(move_req->start_index, move_req->dest_index);

    if (ret >= 0) {
        resp.which_result = zmk_keymap_SetActivePhysicalLayoutResponse_ok_tag;
        resp.result.ok.layers.funcs.encode = encode_keymap_layers;

        raise_zmk_studio_rpc_notification((struct zmk_studio_rpc_notification){
            .notification = KEYMAP_NOTIFICATION(unsaved_changes_status_changed, true)});
    } else {
        LOG_WRN("Failed to move layer: %d", ret);
        resp.which_result = zmk_keymap_MoveLayerResponse_err_tag;
        resp.result.err = zmk_keymap_MoveLayerErrorCode_MOVE_LAYER_ERR_GENERIC;
    }

    return KEYMAP_RESPONSE(move_layer, resp);
}

zmk_studio_Response add_layer(const zmk_studio_Request *req) {
    LOG_DBG("");
    // Use a static here to keep the value valid during serialization
    static zmk_keymap_layer_id_t layer_id = 0;

    zmk_keymap_AddLayerResponse resp = zmk_keymap_AddLayerResponse_init_zero;

    int ret = zmk_keymap_add_layer();

    if (ret >= 0) {
        layer_id = zmk_keymap_layer_index_to_id(ret);

        resp.which_result = zmk_keymap_AddLayerResponse_ok_tag;

        resp.result.ok.index = ret;

        resp.result.ok.has_layer = true;
        resp.result.ok.layer.id = layer_id;

        resp.result.ok.layer.name.funcs.encode = encode_layer_name;
        resp.result.ok.layer.name.arg = &layer_id;

        resp.result.ok.layer.bindings.funcs.encode = encode_layer_bindings;
        resp.result.ok.layer.bindings.arg = &layer_id;

        raise_zmk_studio_rpc_notification((struct zmk_studio_rpc_notification){
            .notification = KEYMAP_NOTIFICATION(unsaved_changes_status_changed, true)});
    } else {
        LOG_WRN("Failed to add layer: %d", ret);
        resp.which_result = zmk_keymap_AddLayerResponse_err_tag;
        switch (ret) {
        case -ENOSPC:
            resp.result.err = zmk_keymap_AddLayerErrorCode_ADD_LAYER_ERR_NO_SPACE;
            break;
        default:
            resp.result.err = zmk_keymap_AddLayerErrorCode_ADD_LAYER_ERR_GENERIC;
            break;
        }
    }

    return KEYMAP_RESPONSE(add_layer, resp);
}

zmk_studio_Response remove_layer(const zmk_studio_Request *req) {
    LOG_DBG("");
    const zmk_keymap_RemoveLayerRequest *rm_req = &req->subsystem.keymap.request_type.remove_layer;

    zmk_keymap_RemoveLayerResponse resp = zmk_keymap_RemoveLayerResponse_init_zero;

    int ret = zmk_keymap_remove_layer(rm_req->layer_index);

    if (ret >= 0) {
        resp.which_result = zmk_keymap_RemoveLayerResponse_ok_tag;

        raise_zmk_studio_rpc_notification((struct zmk_studio_rpc_notification){
            .notification = KEYMAP_NOTIFICATION(unsaved_changes_status_changed, true)});
    } else {
        LOG_WRN("Failed to rm layer: %d", ret);
        resp.which_result = zmk_keymap_RemoveLayerResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_keymap_RemoveLayerErrorCode_REMOVE_LAYER_ERR_INVALID_INDEX;
            break;
        default:
            resp.result.err = zmk_keymap_RemoveLayerErrorCode_REMOVE_LAYER_ERR_GENERIC;
            break;
        }
    }

    return KEYMAP_RESPONSE(remove_layer, resp);
}

zmk_studio_Response restore_layer(const zmk_studio_Request *req) {
    LOG_DBG("");
    const zmk_keymap_RestoreLayerRequest *restore_req =
        &req->subsystem.keymap.request_type.restore_layer;

    zmk_keymap_RestoreLayerResponse resp = zmk_keymap_RestoreLayerResponse_init_zero;

    int ret = zmk_keymap_restore_layer(restore_req->layer_id, restore_req->at_index);

    if (ret >= 0) {
        resp.which_result = zmk_keymap_RemoveLayerResponse_ok_tag;
        resp.result.ok.id = restore_req->layer_id;

        resp.result.ok.name.funcs.encode = encode_layer_name;
        resp.result.ok.name.arg = &restore_req->layer_id;

        resp.result.ok.bindings.funcs.encode = encode_layer_bindings;
        resp.result.ok.bindings.arg = &restore_req->layer_id;

        raise_zmk_studio_rpc_notification((struct zmk_studio_rpc_notification){
            .notification = KEYMAP_NOTIFICATION(unsaved_changes_status_changed, true)});
    } else {
        LOG_WRN("Failed to restore layer: %d", ret);
        resp.which_result = zmk_keymap_RestoreLayerResponse_err_tag;
        switch (ret) {
        case -EINVAL:
            resp.result.err = zmk_keymap_RestoreLayerErrorCode_RESTORE_LAYER_ERR_INVALID_INDEX;
            break;
        default:
            resp.result.err = zmk_keymap_RestoreLayerErrorCode_RESTORE_LAYER_ERR_GENERIC;
            break;
        }
    }

    return KEYMAP_RESPONSE(restore_layer, resp);
}

zmk_studio_Response set_layer_props(const zmk_studio_Request *req) {
    LOG_DBG("");
    const zmk_keymap_SetLayerPropsRequest *set_req =
        &req->subsystem.keymap.request_type.set_layer_props;

    zmk_keymap_SetLayerPropsResponse resp =
        zmk_keymap_SetLayerPropsResponse_SET_LAYER_PROPS_RESP_OK;

    if (strlen(set_req->name) <= 0) {
        return KEYMAP_RESPONSE(set_layer_props, resp);
    }

    int ret = zmk_keymap_set_layer_name(set_req->layer_id, set_req->name, strlen(set_req->name));

    if (ret >= 0) {

        raise_zmk_studio_rpc_notification((struct zmk_studio_rpc_notification){
            .notification = KEYMAP_NOTIFICATION(unsaved_changes_status_changed, true)});
    } else {
        LOG_WRN("Failed to set layer props: %d", ret);
        switch (ret) {
        case -EINVAL:
            resp = zmk_keymap_SetLayerPropsResponse_SET_LAYER_PROPS_RESP_ERR_INVALID_ID;
            break;
        default:
            resp = zmk_keymap_SetLayerPropsResponse_SET_LAYER_PROPS_RESP_ERR_GENERIC;
            break;
        }
    }

    return KEYMAP_RESPONSE(set_layer_props, resp);
}

ZMK_RPC_SUBSYSTEM_HANDLER(keymap, get_keymap, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, set_layer_binding, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, check_unsaved_changes, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, save_changes, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, discard_changes, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, get_physical_layouts, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, set_active_physical_layout, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, move_layer, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, add_layer, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, remove_layer, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, restore_layer, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(keymap, set_layer_props, ZMK_STUDIO_RPC_HANDLER_SECURED);

static int event_mapper(const zmk_event_t *eh, zmk_studio_Notification *n) { return 0; }

ZMK_RPC_EVENT_MAPPER(keymap, event_mapper);
