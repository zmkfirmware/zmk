#pragma once

#include <zmk/keys.h>
#include <zmk/hid.h>

int zmk_endpoints_init();
int zmk_endpoints_send_report(enum zmk_hid_report_changes changes);
int zmk_endpoints_send_key_event(struct zmk_key_event key_event);
