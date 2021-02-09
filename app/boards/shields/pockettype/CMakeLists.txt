#
# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT
#

target_sources_ifdef(CONFIG_SHIELD_POCKETTYPE app PRIVATE pockettype_pwr_led.c)
