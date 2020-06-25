#pragma once

#include <zmk/keys.h>
#include <zmk/hid.h>

int zmk_endpoints_init();
int zmk_endpoints_send_report(u8_t usage_report);
