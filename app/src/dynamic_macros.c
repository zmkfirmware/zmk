#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/dynamic_macros.h>

static int recording_macro_count = 0;

int zmk_recording_macro_count(void) { return recording_macro_count; };
void zmk_recording_macro_count_increase() { recording_macro_count++; }
void zmk_recording_macro_count_decrease() { recording_macro_count--; }
