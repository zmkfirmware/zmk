/* SPDX-License-Identifier: MIT */

#include <pb_encode.h>
#include <zmk/led_matrix_editor.h>
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>
#endif
#include <zmk/behavior.h>
#include <zmk/studio/rpc.h>

ZMK_RPC_SUBSYSTEM(lighting)

static bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    const char *s = *arg;
    return !s || !*s ||
           (pb_encode_tag_for_field(stream, field) && pb_encode_string(stream, s, strlen(s)));
}

static bool encode_profiles(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    const struct led_matrix_independent_group_cfg *group = *arg;
    size_t count = group ? group->profile_count : PROFILE_SLOTS;
    for (size_t i = 0; i < count; i++) {
        if (!group && !profile_available(i)) continue;
        zmk_lighting_Profile p = {
            .id = group ? group->profiles[i].profile_id : display_profiles[i].profile_id,
            .name = {.funcs.encode = encode_string,
                     .arg = (void *)(group ? group->profiles[i].display_name
                                           : display_profiles[i].display_name)},
            .renderer = {.funcs.encode = encode_string,
                         .arg = (void *)renderer_name(group ? group->profiles[i].renderer
                                                            : display_profiles[i].renderer)},
        };
        if (!pb_encode_tag_for_field(stream, field) ||
            !pb_encode_submessage(stream, zmk_lighting_Profile_fields, &p)) {
            return false;
        }
    }
    return true;
}

static bool encode_zones(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    for (size_t i = 0; i < INDEPENDENT_GROUP_COUNT; i++) {
        const struct led_matrix_independent_group_cfg *group = &independent_groups[i];
        zmk_lighting_Zone zone = {
            .id = group->independent_id,
            .name = {.funcs.encode = encode_string, .arg = (void *)group->display_name},
            .profiles = {.funcs.encode = encode_profiles, .arg = (void *)group},
        };
        if (!pb_encode_tag_for_field(stream, field) ||
            !pb_encode_submessage(stream, zmk_lighting_Zone_fields, &zone)) {
            return false;
        }
    }
    return true;
}

static zmk_studio_Response get_catalog(const zmk_studio_Request *req) {
    zmk_lighting_Catalog catalog = {
        .max_speed = LED_MATRIX_SPEED_MAX,
        .editor_version = IS_ENABLED(CONFIG_ZMK_LED_MATRIX_EDITOR) ? 3 : 0,
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
        .peripheral_slots = ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT,
#endif
        .hue_count = LED_MATRIX_HUE_INDEX_MAX + 1,
        .persistent = IS_ENABLED(CONFIG_SETTINGS),
        .profiles.funcs.encode = encode_profiles,
        .zones.funcs.encode = encode_zones,
    };
    return ZMK_RPC_RESPONSE(lighting, catalog, catalog);
}

static bool encode_zone_state(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    return pb_encode_tag_for_field(stream, field) &&
           pb_encode_string(stream, *arg, INDEPENDENT_GROUP_COUNT);
}

static zmk_studio_Response get_state(const zmk_studio_Request *req) {
    // The RPC thread encodes one reply before dispatching the next request.
    static uint8_t zones[LED_MATRIX_INDEPENDENT_PROFILE_SLOT_COUNT];
    memcpy(zones, selected_independent_profile_ids, INDEPENDENT_GROUP_COUNT);
    zmk_lighting_State snapshot = {
        .enabled = zmk_led_matrix_is_on(),
        .brightness = zmk_led_matrix_get_brt(),
        .profile = zmk_led_matrix_get_profile(),
        .speed = zmk_led_matrix_get_spd(),
        .hue = zmk_led_matrix_get_hue_index(),
        .zone_profiles = {.funcs.encode = encode_zone_state, .arg = zones},
    };
    return ZMK_RPC_RESPONSE(lighting, state, snapshot);
}

static bool lighting_binding(const zmk_lighting_Setting *setting,
                             struct zmk_behavior_binding *binding) {
    switch (setting->which_value) {
    case zmk_lighting_Setting_enabled_tag:
        binding->param1 = setting->value.enabled ? LM_ON_CMD : LM_OFF_CMD;
        return true;
    case zmk_lighting_Setting_brightness_tag:
        binding->param1 = LM_BRT_CMD;
        binding->param2 = setting->value.brightness;
        return binding->param2 <= 100;
    case zmk_lighting_Setting_speed_tag:
        binding->param1 = LM_SPS_CMD;
        binding->param2 = setting->value.speed;
        return binding->param2 >= LED_MATRIX_SPEED_MIN && binding->param2 <= LED_MATRIX_SPEED_MAX;
    case zmk_lighting_Setting_hue_tag:
        binding->param1 = LM_HUS_CMD;
        binding->param2 = setting->value.hue;
        return binding->param2 <= LED_MATRIX_HUE_INDEX_MAX;
    case zmk_lighting_Setting_profile_tag:
        binding->param1 = LM_EFS_CMD;
        binding->param2 = setting->value.profile;
        return binding->param2 <= UINT8_MAX && find_display_profile(binding->param2);
    case zmk_lighting_Setting_zone_profile_tag: {
        zmk_lighting_ZoneSelection selection = setting->value.zone_profile;
        if (selection.zone > UINT8_MAX || selection.profile > UINT8_MAX ||
            !find_independent_profile(find_independent_group(selection.zone), selection.profile)) {
            return false;
        }
        binding->param1 = LM_CFS_CMD;
        binding->param2 = LM_CHILD_PROFILE_PACK(selection.zone, selection.profile);
        return true;
    }
    default:
        return false;
    }
}

static zmk_studio_Response set(const zmk_studio_Request *req) {
    struct zmk_behavior_binding binding = {
        .behavior_dev = DEVICE_DT_NAME(DT_INST(0, zmk_behavior_led_matrix)),
    };
    if (!lighting_binding(&req->subsystem.lighting.request_type.set, &binding)) {
        return ZMK_RPC_RESPONSE(lighting, rejected, true);
    }
    struct zmk_behavior_binding_event event = {
        .timestamp = k_uptime_get(),
#if IS_ENABLED(CONFIG_ZMK_SPLIT)
        .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
#endif
    };
    if (zmk_behavior_invoke_binding(&binding, event, true) != ZMK_BEHAVIOR_OPAQUE) {
        return ZMK_RPC_RESPONSE(lighting, rejected, true);
    }
    return get_state(req);
}

ZMK_RPC_SUBSYSTEM_HANDLER(lighting, get_catalog, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(lighting, get_state, ZMK_STUDIO_RPC_HANDLER_SECURED);
ZMK_RPC_SUBSYSTEM_HANDLER(lighting, set, ZMK_STUDIO_RPC_HANDLER_SECURED);

#if IS_ENABLED(CONFIG_ZMK_LED_MATRIX_EDITOR)
static zmk_studio_Response exchange(const zmk_studio_Request *req) {
    const zmk_lighting_Exchange *in = &req->subsystem.lighting.request_type.exchange;
    struct zmk_lighting_packet q, r;
    if (in->packet.size != sizeof(q)) {
        return ZMK_RPC_RESPONSE(lighting, rejected, true);
    }
    memcpy(&q, in->packet.bytes, sizeof(q));
    if (q.op == LM_RUNTIME) return ZMK_RPC_RESPONSE(lighting, rejected, true);
    if (in->side == 0) {
        zmk_lighting_exchange(&q, &r);
    } else {
        r = q;
        int rc = -ENOTSUP;
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
        if (in->side <= ZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT) {
            rc = zmk_lighting_forward(in->side - 1, &r);
        }
#endif
        if (!rc && r.op == LM_LIVE && r.length >= 4) {
            // The active keymap is owned by the central. Layer color and status are side-local.
            r.data[3] = snapshot.active_layer;
        }
        if (rc) {
            r.op = LM_ERROR;
            r.length = 1;
            r.data[0] = rc == -ENOTSUP       ? LM_UNSUPPORTED
                        : rc == -EINPROGRESS ? LM_NOT_READY
                                             : LM_OFFLINE;
        }
    }
    zmk_lighting_Exchange out = {.side = in->side, .packet.size = sizeof(r)};
    memcpy(out.packet.bytes, &r, sizeof(r));
    return ZMK_RPC_RESPONSE(lighting, exchange, out);
}
ZMK_RPC_SUBSYSTEM_HANDLER(lighting, exchange, ZMK_STUDIO_RPC_HANDLER_SECURED);
#endif
