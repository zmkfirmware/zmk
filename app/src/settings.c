#include <device.h>
#include <init.h>
#include <kernel.h>
#include <settings/settings.h>

static int zmk_settings_init(struct device *_arg) { return settings_load(); }

SYS_INIT(zmk_settings_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
