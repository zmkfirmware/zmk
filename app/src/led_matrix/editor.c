/* SPDX-License-Identifier: MIT */
#include <zmk/physical_layouts.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zmk/led_matrix_editor.h>
#include <stdlib.h>
#include <stdio.h>

enum group_kind { MAIN = 1, INDEPENDENT, LAYER, INDICATOR, TIMING };
enum field_id {
    NAME = 1,
    RENDERER,
    BRIGHTNESS,
    PERIOD,
    AFTERGLOW,
    FADE,
    RANGE,
    DISPLAY_TIME,
    TIMEOUT,
    BATTERY_TIME,
    USB_TIME,
    BLE_TIME,
    HID_TIME,
    PAIR_HOLD,
    FIRST_COLOR = 16,
    SECOND_COLOR,
    THIRD_COLOR,
    MEMBERS,
    GROUP_NAME,
    DISABLED,
    EFFECT_STYLE,
    EFFECT_DIRECTION
};
#define INHERIT UINT32_MAX
#define CHILD_PROFILES(n)                                                                          \
    +COND_CODE_1(DT_NODE_HAS_PROP(n, independent_id), (DT_CHILD_NUM_STATUS_OKAY(n)), (0))
#define LOCAL_PROFILES (0 DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, CHILD_PROFILES))
#define BATTERY_BUCKETS MAX(101, DT_PROP_LEN_OR(MATRIX_NODE, battery_level_colors, 0))
#define HID_AVAILABLE (LED_MATRIX_HAS_CENTRAL_STATE && IS_ENABLED(CONFIG_ZMK_HID_INDICATORS))
#define BLE_AVAILABLE (LED_MATRIX_HAS_CENTRAL_STATE && IS_ENABLED(CONFIG_ZMK_BLE))
#define STATUS_WORDS(p, supported, lists)                                                          \
    ((DT_NODE_HAS_PROP(MATRIX_NODE, p) || (supported)) * (lists) * LOGICAL_LED_COUNT)
#define COLOR_WORDS                                                                                \
    ((PROFILE_SLOTS + LAYER_SLOTS + LOCAL_PROFILES + CUSTOM_GROUP_COUNT) * LOGICAL_LED_COUNT +     \
     BATTERY_BUCKETS + STATUS_WORDS(battery_leds, IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING), 2) +   \
     STATUS_WORDS(usb_leds, IS_ENABLED(CONFIG_USB_DEVICE_STACK), 3) +                              \
     STATUS_WORDS(ble_profile_leds, BLE_AVAILABLE, 3) +                                            \
     STATUS_WORDS(hid_caps_leds, HID_AVAILABLE, 2) +                                               \
     STATUS_WORDS(hid_num_leds, HID_AVAILABLE, 2) +                                                \
     STATUS_WORDS(hid_scroll_leds, HID_AVAILABLE, 2) +                                             \
     STATUS_WORDS(hid_compose_leds, HID_AVAILABLE, 2) +                                            \
     STATUS_WORDS(hid_kana_leds, HID_AVAILABLE, 2))
#define GROUP_COUNT_MAX (PROFILE_SLOTS + LOCAL_PROFILES + CUSTOM_GROUP_COUNT + LAYER_SLOTS + 9)
#define FIELD_COUNT_MAX                                                                            \
    (12 * PROFILE_SLOTS + 9 * (LOCAL_PROFILES + CUSTOM_GROUP_COUNT) + 5 * LAYER_SLOTS + 40)
#define NAME_BYTES 64
#define GROUP_KEY(kind, zone, id) (((uint32_t)(kind) << 16) | ((uint32_t)(zone) << 8) | (id))

struct editor_field {
    uint8_t id, width;
    uint16_t capacity;
    void *target;
    size_t *count;
    union {
        uint32_t scalar;
        const void *data;
    } initial;
    size_t initial_count;
    uint32_t fallback;
    void *buffer;
};
struct membership {
    struct led_matrix_independent_group_cfg *zone;
    struct led_zone *indicator;
    struct led_matrix_profile_cfg *main;
    struct led_matrix_layer_color_cfg *layer;
    uint32_t initial[LOGICAL_LED_COUNT], ids[LOGICAL_LED_COUNT];
    uint16_t leds[LOGICAL_LED_COUNT];
    size_t count;
    char name[NAME_BYTES];
    const char *initial_name;
    bool loaded;
};
static struct membership memberships[GROUP_SLOTS + 8];
static struct membership main_members[PROFILE_SLOTS], layer_members[LAYER_SLOTS];
static uint16_t main_member_index[PROFILE_SLOTS][LOGICAL_LED_COUNT];
static struct led_matrix_independent_profile_cfg custom_profiles[CUSTOM_GROUP_COUNT];

struct editor_group {
    struct membership *members;
    uint32_t key;
    struct led_zone zone;
    const char *label;
    uint16_t renderers;
    struct editor_field *fields;
    size_t count;
};
static struct editor_group groups[GROUP_COUNT_MAX];
static struct editor_field fields[FIELD_COUNT_MAX];
static uint32_t editor_colors[COLOR_WORDS];
static char editor_names[MAX(1, PROFILE_SLOTS + LOCAL_PROFILES + CUSTOM_GROUP_COUNT)][NAME_BYTES];
static size_t group_count, field_count, color_offset, name_count;
static uint32_t timing[5] = {STATUS_TIMEOUT_MS,
                             DT_PROP_OR(MATRIX_NODE, battery_timeout_ms, INHERIT),
                             DT_PROP_OR(MATRIX_NODE, usb_timeout_ms, INHERIT),
                             DT_PROP_OR(MATRIX_NODE, ble_timeout_ms, INHERIT),
                             DT_PROP_OR(MATRIX_NODE, hid_timeout_ms, INHERIT)};
static uint8_t source_mask;
static K_MUTEX_DEFINE(editor_mutex);

static struct editor_group *add_group(uint32_t key, struct led_zone zone, const char *label) {
    struct editor_group *g = &groups[group_count++];
    *g = (struct editor_group){
        .key = key, .zone = zone, .label = label, .fields = &fields[field_count]};
    return g;
}
static struct editor_field *add_field(struct editor_group *g, uint8_t id, void *target) {
    struct editor_field *f = &fields[field_count++];
    g->count++;
    f->id = id;
    f->target = target;
    return f;
}
static uint32_t scalar(const struct editor_field *f) {
    uint32_t v = 0;
    memcpy(&v, f->target, f->width);
    return v;
}
static void add_scalar(struct editor_group *g, uint8_t id, void *target, size_t width) {
    struct editor_field *f = add_field(g, id, target);
    f->width = width;
    f->initial.scalar = scalar(f);
}
#define SCALAR(g, id, x) add_scalar(g, id, &(x), sizeof(x))

static void add_name(struct editor_group *g, const char **target) {
    struct editor_field *f = add_field(g, NAME, target);
    f->initial.data = *target;
    f->buffer = editor_names[name_count++];
    f->capacity = NAME_BYTES - 1;
}
static void add_colors(struct editor_group *g, uint8_t id, const uint32_t **target, size_t *count,
                       size_t capacity, uint32_t fallback) {
    struct editor_field *f = add_field(g, id, target);
    f->initial.data = *target;
    f->initial_count = *count;
    f->fallback = fallback;
    f->count = count;
    f->capacity = capacity;
    f->buffer = editor_colors + color_offset;
    color_offset += capacity;
    if (*count)
        memcpy(f->buffer, *target, *count * sizeof(uint32_t));
    *target = f->buffer;
}
static void add_members(struct editor_group *g, struct membership *m) {
    g->members = m;
    struct editor_field *f = add_field(g, MEMBERS, NULL);
    f->initial.data = m->initial;
    f->initial_count = g->zone.count;
    f->count = &m->count;
    f->buffer = m->ids;
    f->capacity = LOGICAL_LED_COUNT;
    m->count = g->zone.count;
    for (size_t i = 0; i < m->count; i++) {
        m->initial[i] = m->ids[i] = g->zone.leds ? g->zone.leds[i] : i;
        m->leds[i] = m->ids[i];
    }
    if (m->zone) {
        f = add_field(g, GROUP_NAME, &m->zone->display_name);
        f->initial.data = m->initial_name;
        f->buffer = m->name;
        f->capacity = NAME_BYTES - 1;
    }
}
static void indicator(uint8_t id, const char *label, struct led_zone *zone,
                      struct led_matrix_color_list_cfg *first,
                      struct led_matrix_color_list_cfg *second,
                      struct led_matrix_color_list_cfg *third) {
    bool supported = id == 0   ? IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
                     : id == 1 ? IS_ENABLED(CONFIG_USB_DEVICE_STACK)
                     : id == 2 ? BLE_AVAILABLE
                               : HID_AVAILABLE;
    if (!zone->count && !supported) {
        return;
    }
    struct editor_group *g = add_group(GROUP_KEY(INDICATOR, 0, id), *zone, label);
    struct membership *m = &memberships[GROUP_SLOTS + id];
    m->indicator = zone;
    add_members(g, m);
    struct led_matrix_color_list_cfg *lists[] = {first, second, third};
    for (size_t i = 0; i < ARRAY_SIZE(lists); i++) {
        if (lists[i]) {
            struct led_matrix_color_list_cfg *c = lists[i];
            add_colors(g, FIRST_COLOR + i, &c->colors, &c->count,
                       id == 0 && i == 2 ? BATTERY_BUCKETS : LOGICAL_LED_COUNT, c->default_color);
        }
    }
    source_mask |= id < 3 ? BIT(id) : LM_STA_HID;
}
static void editor_init_groups(void) {
    for (size_t i = 0; i < PROFILE_SLOTS; i++) {
        struct led_matrix_profile_cfg *p = &display_profiles[i];
        bool custom = i >= DISPLAY_PROFILE_COUNT;
        if (custom)
            *p = (struct led_matrix_profile_cfg){.profile_id = FIRST_CUSTOM_PRESET + i -
                                                               DISPLAY_PROFILE_COUNT,
                                                 .renderer = LED_MATRIX_RENDERER_SOLID,
                                                 .brightness = 100,
                                                 .period_ms = 1000,
                                                 .disabled = true,
                                                 .display_name = "New preset"};
        p->member_index = main_member_index[i];
        for (size_t led = 0; led < LOGICAL_LED_COUNT; led++)
            p->member_index[led] = led;
        struct editor_group *g = add_group(GROUP_KEY(MAIN, 0, p->profile_id),
                                           (struct led_zone){NULL, LOGICAL_LED_COUNT}, NULL);
        g->renderers = BIT_MASK(LED_MATRIX_RENDERER_COUNT);
        if (!led_matrix_has_spatial_grid()) {
            for (uint8_t r = 0; r < LED_MATRIX_RENDERER_COUNT; r++)
                if (effect_requires_grid(r))
                    g->renderers &= ~BIT(r);
        }
        add_name(g, &p->display_name);
        SCALAR(g, RENDERER, p->renderer);
        SCALAR(g, EFFECT_STYLE, p->effect_style);
        SCALAR(g, EFFECT_DIRECTION, p->effect_direction);
        SCALAR(g, BRIGHTNESS, p->brightness);
        SCALAR(g, PERIOD, p->period_ms);
        SCALAR(g, AFTERGLOW, p->keypress_afterglow_ms);
        SCALAR(g, FADE, p->keypress_fadeout_ms);
        SCALAR(g, RANGE, p->keypress_range);
        add_colors(g, FIRST_COLOR, &p->color_map, &p->color_map_count, LOGICAL_LED_COUNT,
                   custom ? 0xffffff : 0);
        main_members[i].main = p;
        add_members(g, &main_members[i]);
        if (custom)
            SCALAR(g, DISABLED, p->disabled);
    }
    for (size_t i = 0; i < GROUP_SLOTS; i++) {
        struct led_matrix_independent_group_cfg *z = &independent_groups[i];
        if (i >= INDEPENDENT_GROUP_COUNT) {
            struct led_matrix_independent_profile_cfg *p =
                &custom_profiles[i - INDEPENDENT_GROUP_COUNT];
            *p = (struct led_matrix_independent_profile_cfg){
                .display_name = "Solid", .brightness = 100, .period_ms = 1000};
            *z = (struct led_matrix_independent_group_cfg){.independent_id = i,
                                                           .display_name = "Custom group",
                                                           .profiles = p,
                                                           .profile_count = 1};
        }
        struct membership *m = &memberships[z->independent_id];
        m->zone = z;
        m->initial_name = z->display_name;
        for (size_t j = 0; j < z->profile_count; j++) {
            struct led_matrix_independent_profile_cfg *p = &z->profiles[j];
            struct editor_group *g =
                add_group(GROUP_KEY(INDEPENDENT, z->independent_id, p->profile_id),
                          (struct led_zone){z->leds, z->led_count}, z->display_name);
            g->renderers = BIT_MASK(4) | BIT(LED_MATRIX_RENDERER_CHASE);
            add_name(g, &p->display_name);
            SCALAR(g, RENDERER, p->renderer);
            SCALAR(g, EFFECT_STYLE, p->effect_style);
            SCALAR(g, EFFECT_DIRECTION, p->effect_direction);
            SCALAR(g, BRIGHTNESS, p->brightness);
            SCALAR(g, PERIOD, p->period_ms);
            add_colors(g, FIRST_COLOR, &p->color_map, &p->color_map_count, LOGICAL_LED_COUNT,
                       i >= INDEPENDENT_GROUP_COUNT ? 0xffffff : 0);
            add_members(g, m);
        }
    }
    for (size_t id = 0; id < EXTRA_LAYER_SLOTS; id++) {
        if (find_layer_color_cfg(id))
            continue;
        layer_colors[runtime_layer_count++] = (struct led_matrix_layer_color_cfg){
            .layer_id = id, .disabled = true, .brightness = 100, .has_explicit_layer_leds = true};
    }
    for (size_t i = 0; i < runtime_layer_count; i++) {
        struct led_matrix_layer_color_cfg *p = &layer_colors[i];
        struct editor_group *g = add_group(GROUP_KEY(LAYER, 0, p->layer_id),
                                           led_matrix_layer_color_zone_for_cfg(p), NULL);
        SCALAR(g, BRIGHTNESS, p->brightness);
        SCALAR(g, DISPLAY_TIME, p->display_time_ms);
        add_colors(g, FIRST_COLOR, &p->layer_colors, &p->layer_color_count, LOGICAL_LED_COUNT,
                   i >= LAYER_COLOR_COUNT ? 0xffffff : 0);
        layer_members[i].layer = p;
        add_members(g, &layer_members[i]);
        SCALAR(g, DISABLED, p->disabled);
    }
    indicator(0, "Battery", &battery_zone, &battery_color_on_cfg, &battery_color_off_cfg,
              &battery_level_color_cfg);
    indicator(1, "USB", &usb_zone, &usb_color_hid_cfg, &usb_color_powered_cfg, &usb_color_none_cfg);
    indicator(2, "Bluetooth", &ble_zone, &ble_color_open_cfg, &ble_color_connected_cfg,
              &ble_color_disconnected_cfg);
#define HID(id, label, n)                                                                          \
    indicator(id, label, &hid_##n##_zone, &hid_##n##_color_on_cfg, &hid_##n##_color_off_cfg, NULL)
    HID(3, "Caps Lock", caps);
    HID(4, "Num Lock", num);
    HID(5, "Scroll Lock", scroll);
    HID(6, "Compose", compose);
    HID(7, "Kana", kana);
    if (source_mask) {
        struct editor_group *g =
            add_group(GROUP_KEY(TIMING, 0, 0), (struct led_zone){0}, "Status timing");
        SCALAR(g, TIMEOUT, timing[0]);
        for (int i = 0; i < 4; i++) {
            if (source_mask & BIT(i)) {
                SCALAR(g, BATTERY_TIME + i, timing[i + 1]);
            }
        }
        if (source_mask & LM_STA_BLE) {
            SCALAR(g, PAIR_HOLD, pairing_hold);
        }
    }
}

/* All TLVs use little-endian integers. Size for both directory reads and atomic zone records. */
#define ZONE_RECORD_SIZE(n)                                                                        \
    +(DT_NODE_HAS_PROP(n, independent_id)                                                          \
          ? (DT_CHILD_NUM_STATUS_OKAY(n) * (2 * NAME_BYTES + 8 * LOGICAL_LED_COUNT + 80))          \
          : 0)
#define EDIT_BYTES                                                                                 \
    MAX(MAX(1024, 128 + 8 * LOGICAL_LED_COUNT + GROUP_COUNT_MAX * (NAME_BYTES + 8) +               \
                      (LOCAL_PROFILES + CUSTOM_GROUP_COUNT) * NAME_BYTES),                         \
        MAX((24 * LOGICAL_LED_COUNT + 4 * BATTERY_BUCKETS + 256),                                  \
            (1 DT_FOREACH_CHILD_STATUS_OKAY(MATRIX_NODE, ZONE_RECORD_SIZE))))
BUILD_ASSERT(EDIT_BYTES <= UINT16_MAX, "Lighting configuration exceeds the transfer offset range");
struct bytes {
    uint8_t *p;
    size_t len, cap;
    bool failed;
};
static void put(struct bytes *b, const void *p, size_t n) {
    if (n > b->cap - b->len) {
        b->failed = true;
        return;
    }
    if (n) {
        memcpy(b->p + b->len, p, n);
    }
    b->len += n;
}
static void put16(struct bytes *b, uint16_t v) {
    uint8_t s[2];
    sys_put_le16(v, s);
    put(b, s, 2);
}
static void put32(struct bytes *b, uint32_t v) {
    uint8_t s[4];
    sys_put_le32(v, s);
    put(b, s, 4);
}
static struct editor_group *group_for(uint32_t key) {
    for (size_t i = 0; i < group_count; i++) {
        if (groups[i].key == key) {
            return &groups[i];
        }
    }
    return NULL;
}
static bool buckets(const struct editor_group *g, const struct editor_field *f) {
    return g->key == GROUP_KEY(INDICATOR, 0, 0) && f->id == THIRD_COLOR;
}
struct field_view {
    const void *p;
    size_t len;
    uint32_t number;
};
static void field_view(const struct editor_group *g, const struct editor_field *f, bool factory,
                       struct field_view *v) {
    if (f->id == NAME || f->id == GROUP_NAME) {
        v->p = factory ? f->initial.data : *(const char **)f->target;
        v->len = strlen(v->p);
    } else if (f->id == MEMBERS) {
        v->p = factory ? f->initial.data : f->buffer;
        v->len = 4 * (factory ? f->initial_count : *f->count);
    } else if (f->count) {
        v->len = (factory ? f->initial_count : *f->count) * 4;
        v->p = factory ? f->initial.data : *(const uint32_t **)f->target;
        if (!v->len && !buckets(g, f)) {
            v->number = f->fallback;
            v->p = &v->number;
            v->len = 4;
        }
    } else {
        v->number = factory ? f->initial.scalar : scalar(f);
        v->p = &v->number;
        v->len = 4;
    }
}
static void tlv(struct bytes *b, uint8_t id, const void *data, size_t n) {
    put(b, &id, 1);
    put16(b, n);
    put(b, data, n);
}
static void write_values(struct bytes *b, const struct editor_group *g, bool factory) {
    for (size_t i = 0; i < g->count; i++) {
        struct field_view v;
        field_view(g, &g->fields[i], factory, &v);
        tlv(b, g->fields[i].id, v.p, v.len);
    }
}
static uint16_t led_at(const struct editor_group *g, size_t i) {
    return g->zone.leds ? g->zone.leds[i] : i;
}
static uint32_t topology(const struct editor_group *g) {
    uint32_t crc = crc32_ieee((const uint8_t *)&g->key, 4);
    for (size_t i = 0; i < g->zone.count; i++) {
        uint16_t led = led_at(g, i);
        crc = crc32_ieee_update(crc, (uint8_t *)&led, 2);
    }
    for (size_t i = 0; i < g->count; i++) {
        if (g->fields[i].id < MEMBERS)
            crc = crc32_ieee_update(crc, &g->fields[i].id, 1);
    }
    return crc;
}
static uint32_t member_topology(const struct editor_group *g) {
    uint32_t crc = topology(g);
    uint16_t count = LOGICAL_LED_COUNT;
    crc = crc32_ieee_update(crc, (const uint8_t *)&count, sizeof(count));
    if (g->members->main || g->members->layer) {
        return crc;
    }
    for (size_t i = 0; i < group_count; i++) {
        if (groups[i].members && groups[i].members->zone) {
            uint32_t factory = topology(&groups[i]);
            crc = crc32_ieee_update(crc, (const uint8_t *)&factory, sizeof(factory));
        }
    }
    return crc;
}
static uint32_t group_revision(const struct editor_group *g, bool factory) {
    uint32_t crc = topology(g);
    for (size_t i = 0; i < g->count; i++) {
        struct field_view v;
        field_view(g, &g->fields[i], factory, &v);
        uint16_t len = v.len;
        crc = crc32_ieee_update(crc, (uint8_t *)&len, 2);
        crc = crc32_ieee_update(crc, v.p, v.len);
    }
    return crc;
}
static bool valid_field(const struct editor_group *g, const struct editor_field *f,
                        const uint8_t *data, size_t len, size_t member_count) {
    if (f->id == MEMBERS) {
        if (len % 4 || len / 4 > LOGICAL_LED_COUNT)
            return false;
        for (size_t i = 0; i < len; i += 4) {
            if (sys_get_le32(data + i) >= LOGICAL_LED_COUNT)
                return false;
            for (size_t j = 0; j < i; j += 4)
                if (sys_get_le32(data + j) == sys_get_le32(data + i))
                    return false;
        }
        return true;
    }
    if (f->id == NAME || f->id == GROUP_NAME) {
        return (len <= f->capacity ||
                (strlen(f->initial.data) == len && !memcmp(data, f->initial.data, len))) &&
               !memchr(data, 0, len);
    }
    if (f->count) {
        if (len % 4 || len / 4 > f->capacity ||
            (!buckets(g, f) && len != 4 && len / 4 != member_count)) {
            return false;
        }
        for (size_t i = 0; i < len; i += 4) {
            if (!led_matrix_is_valid_configured_color(sys_get_le32(data + i))) {
                return false;
            }
        }
        return true;
    }
    if (len != 4) {
        return false;
    }
    uint32_t x = sys_get_le32(data);
    switch (f->id) {
    case RENDERER:
        return x < LED_MATRIX_RENDERER_COUNT && (g->renderers & BIT(x));
    case BRIGHTNESS:
        return x <= 100;
    case PERIOD:
    case TIMEOUT:
        return x <= UINT16_MAX;
    case BATTERY_TIME:
    case USB_TIME:
    case BLE_TIME:
    case HID_TIME:
        return x == INHERIT || x <= UINT16_MAX;
    case DISABLED:
    case PAIR_HOLD:
        return x <= 1;
    default:
        return x <= INT32_MAX;
    }
}
struct parsed_field {
    const uint8_t *p;
    size_t len;
};
static int index_fields(const struct editor_group *g, const uint8_t *p, size_t len,
                        struct parsed_field *values) {
    while (len) {
        if (len < 3) {
            return -EINVAL;
        }
        uint8_t id = p[0];
        size_t n = sys_get_le16(p + 1);
        p += 3;
        len -= 3;
        if (n > len) {
            return -EINVAL;
        }
        size_t i = 0;
        while (i < g->count && g->fields[i].id != id) {
            i++;
        }
        if (i == g->count || values[i].p) {
            return -EINVAL;
        }
        values[i] = (struct parsed_field){p, n};
        p += n;
        len -= n;
    }
    return 0;
}

static int parse_fields(const struct editor_group *g, const uint8_t *p, size_t len, bool full,
                        bool legacy, struct parsed_field *values) {
    if (index_fields(g, p, len, values))
        return -EINVAL;
    size_t count = g->zone.count;
    for (size_t i = 0; i < g->count; i++) {
        if (g->fields[i].id == MEMBERS && values[i].p)
            count = values[i].len / 4;
    }
    unsigned renderer = 0, style = 0, direction = 0;
    static const uint32_t zero;
    for (size_t i = 0; i < g->count; i++) {
        const struct editor_field *f = &g->fields[i];
        if (legacy && !values[i].p && (f->id == EFFECT_STYLE || f->id == EFFECT_DIRECTION))
            values[i] = (struct parsed_field){(const uint8_t *)&zero, 4};
        if ((full && !values[i].p) ||
            (values[i].p && !valid_field(g, &g->fields[i], values[i].p, values[i].len, count))) {
            return -EINVAL;
        }
        if (f->id == RENDERER || f->id == EFFECT_STYLE || f->id == EFFECT_DIRECTION) {
            uint32_t n = values[i].p ? sys_get_le32(values[i].p) : f->initial.scalar;
            if (f->id == RENDERER)
                renderer = n;
            if (f->id == EFFECT_STYLE)
                style = n;
            if (f->id == EFFECT_DIRECTION)
                direction = n;
        }
    }
    return effect_options_valid(renderer, style, direction) ? 0 : -EINVAL;
}
static void install_values(struct editor_group *g, const struct parsed_field *v) {
    unsigned int irq = irq_lock();
    for (size_t i = 0; i < g->count; i++) {
        struct editor_field *f = &g->fields[i];
        struct field_view initial;
        field_view(g, f, true, &initial);
        const void *p = v[i].p ? v[i].p : initial.p;
        size_t n = v[i].p ? v[i].len : initial.len;
        if (f->id == NAME || f->id == GROUP_NAME) {
            if (!v[i].p || (n == initial.len && !memcmp(p, initial.p, n))) {
                *(const char **)f->target = initial.p;
                continue;
            }
            memcpy(f->buffer, p, n);
            ((char *)f->buffer)[n] = 0;
            *(const char **)f->target = f->buffer;
        } else if (f->id == MEMBERS) {
            struct membership *m = g->members;
            m->count = n / 4;
            memcpy(m->ids, p, n);
            for (size_t j = 0; j < m->count; j++)
                m->leds[j] = m->ids[j];
            if (m->main) {
                for (size_t led = 0; led < LOGICAL_LED_COUNT; led++)
                    m->main->member_index[led] = LM_NOLED;
                for (size_t j = 0; j < m->count; j++)
                    m->main->member_index[m->leds[j]] = j;
            }
            if (m->layer) {
                m->layer->layer_leds = m->leds;
                m->layer->layer_led_count = m->count;
                m->layer->has_explicit_layer_leds = true;
            }
            if (m->indicator)
                *m->indicator = (struct led_zone){m->leds, m->count};
            if (m->zone) {
                m->zone->leds = m->leds;
                m->zone->led_count = m->count;
            }
        } else if (f->count) {
            memcpy(f->buffer, p, n);
            *f->count = n / 4;
        } else {
            memcpy(f->target, p, f->width);
        }
    }
    manual_timeout = timing[0];
    for (size_t i = 0; i < 4; i++) {
        status_source_descriptors[i].timeout_ms =
            timing[i + 1] == INHERIT ? timing[0] : timing[i + 1];
    }
    atomic_inc(&led_matrix_output_generation);
    irq_unlock(irq);
}
static void config_changed(const struct editor_group *g) {
    if ((g->key >> 16) == TIMING || (g->key >> 16) == INDICATOR) {
        led_matrix_refresh_status_snapshot(&snapshot);
        led_matrix_restore_base_runtime_overlays();
    }
    if ((g->key >> 16) == LAYER && snapshot.active_layer == (g->key & 0xff)) {
        led_matrix_refresh_layer_color(true);
    }
    if ((g->key >> 16) == MAIN &&
        (state.selected_profile_id == (g->key & 0xff) || !selected_profile_cfg())) {
        if (!find_display_profile(state.selected_profile_id)) {
            state.selected_profile_id = first_available_profile();
            led_matrix_save_state();
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
            zmk_lighting_sync_runtime();
#endif
        }
        ripple_state.active = false;
        effect_reset_input();
        led_matrix_reset_rain_state();
        clear_keypress_release_state();
    }
    led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT | LED_MATRIX_SERVICE_RENDER);
    (void)led_matrix_raise_state_changed(ZMK_LED_MATRIX_STATE_CHANGE_PROFILE |
                                         ZMK_LED_MATRIX_STATE_CHANGE_STATUS);
}
static uint8_t transfer[EDIT_BYTES + 5], record[EDIT_BYTES + 5];
static bool storage_write_active;
static struct {
    uint16_t ticket, size, next;
    uint8_t mode;
    struct editor_group *group;
    uint32_t revision;
} transaction;
/* A zone's profiles share one settings record. This keeps the membership and all
 * per-LED color lists consistent across power loss, including when a sibling
 * preset has colors in a different order from its factory configuration. */
static int owner_of(uint16_t led);
static int zone_record(const struct editor_group *target, const struct parsed_field *v,
                       struct bytes *out) {
    struct membership *m = target->members;
    const uint8_t *ids = NULL;
    size_t count = 0;
    struct field_view name = {0};
    for (size_t i = 0; i < target->count; i++) {
        const struct editor_field *f = &target->fields[i];
        struct field_view value;
        field_view(target, f, true, &value);
        if (v[i].p) {
            value.p = v[i].p;
            value.len = v[i].len;
        }
        if (f->id == MEMBERS) {
            ids = value.p;
            count = value.len / 4;
        }
        if (f->id == GROUP_NAME)
            name = value;
    }
    if (m->zone) {
        for (size_t i = 0; i < count; i++) {
            int owner = owner_of(sys_get_le32(ids + 4 * i));
            if (owner >= 0 && owner != m->zone->independent_id)
                return -EAGAIN;
        }
    }
    for (size_t j = 0; j < group_count; j++) {
        struct editor_group *g = &groups[j];
        if (g->members != m)
            continue;
        put32(out, g->key);
        put32(out, member_topology(g));
        size_t offset = out->len;
        put16(out, 0);
        for (size_t i = 0; i < g->count; i++) {
            const struct editor_field *f = &g->fields[i];
            struct field_view value;
            field_view(g, f, g == target, &value);
            if (g == target && v[i].p) {
                value.p = v[i].p;
                value.len = v[i].len;
            } else if (g != target && f->id == MEMBERS) {
                value.p = ids;
                value.len = 4 * count;
            } else if (g != target && f->id == GROUP_NAME) {
                value = name;
            } else if (g != target && f->id == FIRST_COLOR && value.len > 4) {
                if (!count) {
                    tlv(out, f->id, value.p, 4);
                    continue;
                }
                put(out, &f->id, 1);
                put16(out, 4 * count);
                const uint32_t *colors = value.p;
                for (size_t k = 0; k < count; k++) {
                    uint32_t led = sys_get_le32(ids + 4 * k);
                    size_t index = 0;
                    while (index < m->count && m->ids[index] != led)
                        index++;
                    put32(out, colors[index < m->count ? index : 0]);
                }
                continue;
            }
            tlv(out, f->id, value.p, value.len);
        }
        if (out->failed)
            return -ENOSPC;
        sys_put_le16(out->len - offset - 2, out->p + offset);
    }
    return 0;
}

static int read_zone_record(struct editor_group *target, const uint8_t *p, size_t len,
                            bool install, bool legacy) {
    size_t expected = target->members->zone ? target->members->zone->profile_count : 1;
    for (size_t j = 0; j < group_count; j++) {
        struct editor_group *g = &groups[j];
        if (g->members != target->members)
            continue;
        if (len < 10 || sys_get_le32(p) != g->key ||
            (!install && sys_get_le32(p + 4) != member_topology(g)))
            return -EINVAL;
        size_t n = sys_get_le16(p + 8);
        if (n > len - 10)
            return -EINVAL;
        struct parsed_field fields[12] = {0};
        if (install) {
            /* This immutable record was validated before flash I/O. Keep CRCs
             * and the duplicate-member scan outside the render critical section. */
            if (legacy ? parse_fields(g, p + 10, n, true, true, fields)
                       : index_fields(g, p + 10, n, fields))
                return -EINVAL;
            install_values(g, fields);
        } else if (parse_fields(g, p + 10, n, true, legacy, fields)) {
            return -EINVAL;
        }
        p += n + 10;
        len -= n + 10;
        expected--;
    }
    return len || expected ? -EINVAL : 0;
}

static int save_group(struct editor_group *g, const uint8_t *data, size_t len, bool restore) {
    struct parsed_field v[12] = {0};
    int rc = parse_fields(g, data, len, !restore, false, v);
    if (rc) {
        return rc;
    }
    struct bytes b = {.p = record, .cap = sizeof(record)};
    uint32_t storage_key = g->key;
    if (g->members) {
        struct membership *m = g->members;
        uint8_t version = m->main || m->zone ? 4 : m->layer ? 3 : 2;
        put(&b, &version, 1);
        if (m->zone)
            storage_key &= ~0xffU;
        if (!restore && m->zone && m->zone->independent_id >= INDEPENDENT_GROUP_COUNT) {
            for (size_t i = 0; i < g->count; i++) {
                if ((g->fields[i].id == MEMBERS || g->fields[i].id == GROUP_NAME) && !v[i].len)
                    return -EINVAL;
            }
        }
        rc = zone_record(g, v, &b);
        if (rc)
            return rc;
        rc = read_zone_record(g, record + 1, b.len - 1, false, false);
        if (rc)
            return rc;
    } else {
        uint8_t version = 1;
        put(&b, &version, 1);
        put32(&b, topology(g));
        for (size_t i = 0; i < g->count; i++) {
            if (!v[i].p) {
                continue;
            }
            struct field_view factory;
            field_view(g, &g->fields[i], true, &factory);
            if (v[i].len == factory.len && !memcmp(v[i].p, factory.p, factory.len)) {
                v[i].p = NULL;
                continue;
            }
            tlv(&b, g->fields[i].id, v[i].p, v[i].len);
        }
    }
    if (b.failed) {
        return -ENOSPC;
    }
    char key[32];
    snprintf(key, sizeof(key), "lm_editor/%06x", (unsigned)storage_key);
    /* The settings loader takes the settings lock before calling editor_load.
     * Release the editor lock during flash I/O and reserve this transfer until it returns. */
    memcpy(transfer, record, b.len);
    storage_write_active = true;
    k_mutex_unlock(&editor_mutex);
    bool remove = (!g->members && b.len == 5) ||
                  (restore && g->members && (g->members->main || g->members->layer)) ||
                  (restore && g->members && g->members->zone &&
                   g->members->zone->independent_id >= INDEPENDENT_GROUP_COUNT);
    rc = remove ? settings_delete(key) : settings_save_one(key, transfer, b.len);
    k_mutex_lock(&editor_mutex, K_FOREVER);
    storage_write_active = false;
    if (!rc && g->members) {
        unsigned int irq = irq_lock();
        rc = read_zone_record(g, transfer + 1, b.len - 1, true, false);
        irq_unlock(irq);
        if (!rc)
            config_changed(g);
        return rc;
    }
    if (!rc) {
        memset(v, 0, sizeof(v));
        rc = parse_fields(g, transfer + 5, b.len - 5, false, false, v);
    }
    if (rc) {
        return rc;
    }
    install_values(g, v);
    config_changed(g);
    return 0;
}
static bool editor_loaded;
static int editor_load(const char *name, size_t len, settings_read_cb read, void *arg) {
    if (!name || strlen(name) != 6 || len < 5 || len > sizeof(record)) {
        return -EINVAL;
    }
    char *end;
    uint32_t key = strtoul(name, &end, 16);
    struct editor_group *g = group_for(key);
    if (*end || !g) {
        return 0; // A firmware update may remove a group.
    }
    k_mutex_lock(&editor_mutex, K_FOREVER);
    struct parsed_field v[12] = {0};
    if (read(arg, record, len) == len) {
        bool legacy = record[0] < 4;
        if (g->members &&
            (record[0] == (g->members->main || g->members->layer ? 3 : 2) ||
             (record[0] == 4 && (g->members->main || g->members->zone))) &&
            !read_zone_record(g, record + 1, len - 1, false, legacy)) {
            read_zone_record(g, record + 1, len - 1, true, legacy);
            g->members->loaded = true;
            editor_loaded = true;
        } else if (record[0] == 1 && (!g->members || !g->members->loaded) &&
                   sys_get_le32(record + 1) == topology(g) &&
                   !parse_fields(g, record + 5, len - 5, false, true, v)) {
            install_values(g, v);
            editor_loaded = true;
        }
    }
    k_mutex_unlock(&editor_mutex);
    return 0;
}
static int editor_load_commit(void) {
    led_matrix_sanitize_runtime_state();
    if (editor_loaded) {
        editor_loaded = false;
        led_matrix_restore_base_runtime_overlays();
        led_matrix_queue_service_work(LED_MATRIX_SERVICE_SYNC_OUTPUT | LED_MATRIX_SERVICE_RENDER);
    }
    return 0;
}
SETTINGS_STATIC_HANDLER_DEFINE(lm_editor, "lm_editor", NULL, editor_load, editor_load_commit, NULL);

static void short_string(struct bytes *b, const char *s) {
    uint8_t n = s ? strnlen(s, NAME_BYTES - 1) : 0;
    put(b, &n, 1);
    put(b, s, n);
}
static int owner_of(uint16_t led) {
    for (size_t z = 0; z < GROUP_SLOTS; z++) {
        struct membership *m = &memberships[z];
        for (size_t i = 0; i < m->count; i++)
            if (m->ids[i] == led)
                return z;
    }
    return -1;
}

static void directory(struct bytes *b, uint32_t layout, bool geometry) {
    uint8_t version = geometry ? 4 : 3;
    put(b, &version, 1);
    put(b, &source_mask, 1);
    short_string(b, CONFIG_BOARD);
    uint8_t slots[] = {INDEPENDENT_GROUP_COUNT, CUSTOM_GROUP_COUNT, FIRST_CUSTOM_PRESET,
                       CUSTOM_PRESET_COUNT};
    put(b, slots, sizeof(slots));
    put16(b, LOGICAL_LED_COUNT);
    if (geometry) {
        put16(b, grid_row_count);
        put16(b, grid_col_count);
        uint8_t speed = led_matrix_default_speed();
        put(b, &speed, 1);
    }
    static uint32_t map[ZMK_KEYMAP_LEN];
    memset(map, 0xff, sizeof(map));
#if HAS_MATRIX_LED_GRID
    const struct zmk_physical_layout *const *layouts;
    size_t count = zmk_physical_layouts_get_list(&layouts);
    for (size_t i = 0; layout < count && i < count; i++) {
        if (layouts[i]->matrix_transform == LED_MATRIX_TRANSFORM) {
            if (zmk_physical_layouts_get_position_map(i, layout, ARRAY_SIZE(map), map) < 0)
                memset(map, 0xff, sizeof(map));
            break;
        }
    }
#endif
    for (size_t led = 0; led < LOGICAL_LED_COUNT; led++) {
        uint16_t pos = UINT16_MAX;
        for (size_t j = 0; j < ARRAY_SIZE(map); j++) {
            if (map[j] < ARRAY_SIZE(position_to_led) && position_to_led[map[j]] == led) {
                pos = j;
                break;
            }
        }
        put16(b, pos);
        put16(b, owner_of(led));
        if (geometry) {
            uint16_t cell = led_to_cell[led];
            put16(b, cell < grid_cell_count ? cell_row(cell) : UINT16_MAX);
            put16(b, cell < grid_cell_count ? cell_col(cell) : UINT16_MAX);
        }
    }
    for (size_t i = 0; i < group_count; i++) {
        const struct editor_group *g = &groups[i];
        if (g->members && g->members->zone &&
            g->members->zone->independent_id >= INDEPENDENT_GROUP_COUNT && !g->members->count)
            continue;
        if (g->members && g->members->main && g->members->main->disabled)
            continue;
        const char *label =
            g->members && g->members->zone ? g->members->zone->display_name : g->label;
        if ((g->key >> 16) == MAIN) {
            label = *(const char **)g->fields[0].target;
        }
#if LED_MATRIX_HAS_CENTRAL_STATE
        if ((g->key >> 16) == LAYER && (g->key & 255) < ZMK_KEYMAP_LAYERS_LEN) {
            label = zmk_keymap_layer_name(g->key & 255);
        }
#endif
        put32(b, g->key);
        short_string(b, label);
        if ((g->key >> 16) == INDEPENDENT) {
            short_string(b, *(const char **)g->fields[0].target);
        }
    }
}
static int handle_editor_packet(const struct zmk_lighting_packet *q,
                                struct zmk_lighting_packet *r) {
    if (storage_write_active) {
        return -EBUSY;
    }
    if (!group_count) {
        return -ENODEV;
    }
    if (q->length > sizeof(q->data)) {
        return -EINVAL;
    }
    uint32_t key = q->length >= 4 ? sys_get_le32(q->data) : 0;
    struct editor_group *g = group_for(key);
    switch (q->op) {
    case LM_DIRECTORY:
    case LM_FACTORY_GROUP:
    case LM_GROUP: {
        struct bytes b = {transfer, 0, sizeof(transfer)};
        transaction.mode = 0;
        if (q->op == LM_DIRECTORY) {
            directory(&b, key, q->length >= 8 && sys_get_le32(q->data + 4) >= 4);
        } else {
            if (!g) {
                return -EINVAL;
            }
            put32(&b, group_revision(g, false));
            put32(&b, group_revision(g, true));
            put16(&b, g->renderers);
            bool factory = q->op == LM_FACTORY_GROUP;
            size_t count = g->members && !factory ? g->members->count : g->zone.count;
            put16(&b, count);
            for (size_t i = 0; i < count; i++) {
                put16(&b, g->members && !factory ? g->members->ids[i] : led_at(g, i));
            }
            write_values(&b, g, factory);
        }
        if (b.failed) {
            return -ENOSPC;
        }
        transaction.ticket = q->ticket;
        transaction.size = b.len;
        transaction.mode = LM_READ;
        r->offset = b.len;
        return 0;
    }
    case LM_READ:
        if (transaction.mode != LM_READ || q->ticket != transaction.ticket) {
            return -ESTALE;
        }
        if (q->offset >= transaction.size) {
            return -EINVAL;
        }
        r->offset = q->offset;
        r->length = MIN(sizeof(r->data), transaction.size - q->offset);
        memcpy(r->data, transfer + q->offset, r->length);
        return 0;
    case LM_BEGIN:
    case LM_RESTORE:
        if (q->length != 8 || !g) {
            return -EINVAL;
        }
        if (group_revision(g, false) != sys_get_le32(q->data + 4)) {
            return -EAGAIN;
        }
        transaction.mode = 0;
        if (q->op == LM_RESTORE) {
            return save_group(g, NULL, 0, true);
        }
        if (!q->offset || q->offset > EDIT_BYTES) {
            return -ENOSPC;
        }
        transaction.ticket = q->ticket;
        transaction.mode = LM_APPEND;
        transaction.group = g;
        transaction.size = q->offset;
        transaction.next = 0;
        transaction.revision = sys_get_le32(q->data + 4);
        return 0;
    case LM_APPEND:
    case LM_COMMIT:
        if (transaction.mode != LM_APPEND || transaction.ticket != q->ticket) {
            return -ESTALE;
        }
        if (q->op == LM_APPEND) {
            if (!q->length || q->offset != transaction.next ||
                q->length > transaction.size - transaction.next) {
                return -EINVAL;
            }
            memcpy(transfer + q->offset, q->data, q->length);
            transaction.next += q->length;
            return 0;
        }
        transaction.mode = 0;
        if (transaction.next != transaction.size) {
            return -EINVAL;
        }
        if (group_revision(transaction.group, false) != transaction.revision) {
            return -EAGAIN;
        }
        return save_group(transaction.group, transfer, transaction.size, false);
    case LM_SHOW:
        if (q->length != 1 || !q->data[0] || (q->data[0] & ~source_mask)) {
            return -EINVAL;
        }
        return zmk_led_matrix_show_status(q->data[0]);
    case LM_LIVE: {
        struct zmk_led_matrix_state s;
        zmk_led_matrix_get_state(&s);
        r->data[0] = s.effective_source;
        r->data[1] = s.effective_status_sources_mask;
        r->data[2] = s.effective_layer_color_layer_id;
        r->data[3] = snapshot.active_layer;
#if !LED_MATRIX_HAS_CENTRAL_STATE
        if (!has_central_layer) r->data[3] = ZMK_KEYMAP_LAYER_ID_INVAL;
#endif
        r->data[4] = snapshot.battery_soc;
        r->data[5] = snapshot.usb_state;
        r->data[6] = snapshot.ble_profile;
        r->data[7] = snapshot.ble_open ? 2 : snapshot.ble_connected ? 1 : 0;
        r->data[8] = snapshot.hid;
        r->data[9] =
            (IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING) ? LM_STA_BATTERY : 0) |
            (IS_ENABLED(CONFIG_USB_DEVICE_STACK) ? LM_STA_USB : 0) |
            (LED_MATRIX_HAS_CENTRAL_STATE && IS_ENABLED(CONFIG_ZMK_BLE) ? LM_STA_BLE : 0) |
            (LED_MATRIX_HAS_CENTRAL_STATE && IS_ENABLED(CONFIG_ZMK_HID_INDICATORS) ? LM_STA_HID
                                                                                   : 0);
        r->length = 10;
        return 0;
    }
    default:
        return -ENOTSUP;
    }
}
void zmk_lighting_exchange(const struct zmk_lighting_packet *q, struct zmk_lighting_packet *r) {
    *r = (struct zmk_lighting_packet){.op = q->op, .sequence = q->sequence, .ticket = q->ticket};
    k_mutex_lock(&editor_mutex, K_FOREVER);
    int rc = handle_editor_packet(q, r);
    k_mutex_unlock(&editor_mutex);
    if (rc) {
        r->op = LM_ERROR;
        r->length = 1;
        r->data[0] = rc == -EINVAL                    ? LM_INVALID
                     : rc == -ESTALE                  ? LM_EXPIRED
                     : rc == -EAGAIN                  ? LM_CONFLICT
                     : rc == -ENOSPC || rc == -ENOMEM ? LM_FULL
                     : rc == -ENOTSUP                 ? LM_UNSUPPORTED
                                                      : LM_IO;
    }
}
