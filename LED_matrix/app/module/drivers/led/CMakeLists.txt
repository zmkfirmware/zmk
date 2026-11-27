# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_ZMK_LED_COMPOSITE led_composite.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_LED_MOCK led_mock.c)
zephyr_library_sources(is31/power.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_IS31FL3737 is31/is31fl3737.c)
