#pragma once

#include <zephyr.h>
#include <dt-bindings/zmk/keys.h>

typedef u64_t zmk_key;

struct zmk_key_event
{
    zmk_key key;
    bool pressed;
};