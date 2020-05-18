
#pragma once

#include <zmk/keys.h>
#include <zmk/hid.h>

int zmk_hog_init();

int zmk_hog_send_report(struct zmk_hid_report *report);
