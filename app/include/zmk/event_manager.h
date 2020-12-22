/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stddef.h>
#include <kernel.h>
#include <zephyr/types.h>

struct zmk_event_type {
    const char *name;
};

struct zmk_event_header {
    const struct zmk_event_type *event;
    uint8_t last_listener_index;
};

#define ZMK_EV_EVENT_HANDLED 1
#define ZMK_EV_EVENT_CAPTURED 2

typedef int (*zmk_listener_callback_t)(const struct zmk_event_header *eh);
struct zmk_listener {
    zmk_listener_callback_t callback;
};

struct zmk_event_subscription {
    const struct zmk_event_type *event_type;
    const struct zmk_listener *listener;
};

#define ZMK_EVENT_DECLARE(event_type)                                                              \
    struct event_type *new_##event_type();                                                         \
    bool is_##event_type(const struct zmk_event_header *eh);                                       \
    struct event_type *cast_##event_type(const struct zmk_event_header *eh);                       \
    extern const struct zmk_event_type zmk_event_##event_type;

#define ZMK_EVENT_IMPL(event_type)                                                                 \
    const struct zmk_event_type zmk_event_##event_type = {.name = STRINGIFY(event_type)};          \
    const struct zmk_event_type *zmk_event_ref_##event_type __used                                 \
        __attribute__((__section__(".event_type"))) = &zmk_event_##event_type;                     \
    struct event_type *new_##event_type() {                                                        \
        struct event_type *ev = (struct event_type *)k_malloc(sizeof(struct event_type));          \
        ev->header.event = &zmk_event_##event_type;                                                \
        return ev;                                                                                 \
    };                                                                                             \
    bool is_##event_type(const struct zmk_event_header *eh) {                                      \
        return eh->event == &zmk_event_##event_type;                                               \
    };                                                                                             \
    struct event_type *cast_##event_type(const struct zmk_event_header *eh) {                      \
        return (struct event_type *)eh;                                                            \
    };

#define ZMK_LISTENER(mod, cb) const struct zmk_listener zmk_listener_##mod = {.callback = cb};

#define ZMK_SUBSCRIPTION(mod, ev_type)                                                             \
    const Z_DECL_ALIGN(struct zmk_event_subscription)                                              \
        _CONCAT(_CONCAT(zmk_event_sub_, mod), ev_type) __used                                      \
        __attribute__((__section__(".event_subscription"))) = {                                    \
            .event_type = &zmk_event_##ev_type,                                                    \
            .listener = &zmk_listener_##mod,                                                       \
    };

#define ZMK_EVENT_RAISE(ev) zmk_event_manager_raise((struct zmk_event_header *)ev);

#define ZMK_EVENT_RAISE_AFTER(ev, mod)                                                             \
    zmk_event_manager_raise_after((struct zmk_event_header *)ev, &zmk_listener_##mod);

#define ZMK_EVENT_RAISE_AT(ev, mod)                                                                \
    zmk_event_manager_raise_at((struct zmk_event_header *)ev, &zmk_listener_##mod);

#define ZMK_EVENT_RELEASE(ev) zmk_event_manager_release((struct zmk_event_header *)ev);

int zmk_event_manager_raise(struct zmk_event_header *event);
int zmk_event_manager_raise_after(struct zmk_event_header *event,
                                  const struct zmk_listener *listener);
int zmk_event_manager_raise_at(struct zmk_event_header *event, const struct zmk_listener *listener);
int zmk_event_manager_release(struct zmk_event_header *event);
