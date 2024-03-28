# Copyright (c) 2021 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library_amend()

zephyr_library_sources_ifdef(CONFIG_IL0323 il0323.c)
zephyr_library_sources_ifdef(CONFIG_LPM009M360A lpm009m360a.c)