#pragma once

#include <zmk/keys.h>

int zmk_events_modifiers_pressed(zmk_mod_flags modifiers);
int zmk_events_modifiers_released(zmk_mod_flags modifiers);

// TODO: Encoders?
// TODO: Sensors?