# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_EN11 en11.c)
zephyr_library_sources_ifdef(CONFIG_EN11_TRIGGER en11_trigger.c)
