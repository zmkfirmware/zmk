#pragma once

#include <zephyr.h>

typedef u64_t zmk_key;

struct zmk_key_event
{
    zmk_key key;
    bool pressed;
};