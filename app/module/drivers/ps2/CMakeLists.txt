# Copyright (c) 2023 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library_sources_ifdef(CONFIG_PS2_GPIO		ps2_gpio.c)
zephyr_library_sources_ifdef(CONFIG_PS2_UART	    ps2_uart.c)
