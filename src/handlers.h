#pragma once

#include "keymap.h"
#include <dt-bindings/zmk/keys.h>

struct zmk_key_event
{
    zmk_key key;
    bool pressed;
};

void zmk_handle_key(struct zmk_key_event key_event);
