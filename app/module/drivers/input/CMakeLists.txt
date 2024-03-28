# Copyright (c) 2022 The ZMK Contributors
 # SPDX-License-Identifier: MIT

 zephyr_library_amend()

 zephyr_library_sources_ifdef(CONFIG_ZMK_INPUT_MOUSE_PS2 input_mouse_ps2.c)
