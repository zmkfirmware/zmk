# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

target_sources(app PRIVATE main.c)
target_sources(app PRIVATE theme.c)
target_sources_ifdef(CONFIG_ZMK_DISPLAY_BLANK_ON_IDLE app PRIVATE idle.c)
target_sources_ifdef(CONFIG_ZMK_DISPLAY_PAIRING_SCREEN_BUILT_IN app PRIVATE pairing_screen.c)
target_sources_ifdef(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN app PRIVATE status_screen.c)

add_subdirectory(widgets/)
