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

typedef struct {
    const struct zmk_event_type *event;
    uint8_t last_listener_index;
} zmk_event_t;

#define ZMK_EV_EVENT_BUBBLE 0
#define ZMK_EV_EVENT_HANDLED 1
#define ZMK_EV_EVENT_CAPTURED 2

typedef int (*zmk_listener_callback_t)(const zmk_event_t *eh);
struct zmk_listener {
    zmk_listener_callback_t callback;
};

struct zmk_event_subscription {
    const struct zmk_event_type *event_type;
    const struct zmk_listener *listener;
};

#define ZMK_EVENT_DECLARE(event_type)                                                              \
    struct event_type##_event {                                                                    \
        zmk_event_t header;                                                                        \
        struct event_type data;                                                                    \
    };                                                                                             \
    struct event_type##_event *new_##event_type(struct event_type);                                \
    struct event_type *as_##event_type(const zmk_event_t *eh);                                     \
    extern const struct zmk_event_type zmk_event_##event_type;

#define ZMK_EVENT_IMPL(event_type)                                                                 \
    const struct zmk_event_type zmk_event_##event_type = {.name = STRINGIFY(event_type)};          \
    const struct zmk_event_type *zmk_event_ref_##event_type __used                                 \
        __attribute__((__section__(".event_type"))) = &zmk_event_##event_type;                     \
    struct event_type##_event *new_##event_type(struct event_type data) {                          \
        struct event_type##_event *ev =                                                            \
            (struct event_type##_event *)k_malloc(sizeof(struct event_type##_event));              \
        ev->header.event = &zmk_event_##event_type;                                                \
        ev->data = data;                                                                           \
        return ev;                                                                                 \
    };                                                                                             \
    struct event_type *as_##event_type(const zmk_event_t *eh) {                                    \
        return (eh->event == &zmk_event_##event_type) ? &((struct event_type##_event *)eh)->data   \
                                                      : NULL;                                      \
    };

#define ZMK_LISTENER(mod, cb) const struct zmk_listener zmk_listener_##mod = {.callback = cb};

#define ZMK_SUBSCRIPTION(mod, ev_type)                                                             \
    const Z_DECL_ALIGN(struct zmk_event_subscription)                                              \
        _CONCAT(_CONCAT(zmk_event_sub_, mod), ev_type) __used                                      \
        __attribute__((__section__(".event_subscription"))) = {                                    \
            .event_type = &zmk_event_##ev_type,                                                    \
            .listener = &zmk_listener_##mod,                                                       \
    };

#define ZMK_EVENT_RAISE(ev) zmk_event_manager_raise((zmk_event_t *)ev);

#define ZMK_EVENT_RAISE_AFTER(ev, mod)                                                             \
    zmk_event_manager_raise_after((zmk_event_t *)ev, &zmk_listener_##mod);

#define ZMK_EVENT_RAISE_AT(ev, mod)                                                                \
    zmk_event_manager_raise_at((zmk_event_t *)ev, &zmk_listener_##mod);

#define ZMK_EVENT_RELEASE(ev) zmk_event_manager_release((zmk_event_t *)ev);

#define ZMK_EVENT_FREE(ev) k_free((void *)ev);

int zmk_event_manager_raise(zmk_event_t *event);
int zmk_event_manager_raise_after(zmk_event_t *event, const struct zmk_listener *listener);
int zmk_event_manager_raise_at(zmk_event_t *event, const struct zmk_listener *listener);
int zmk_event_manager_release(zmk_event_t *event);