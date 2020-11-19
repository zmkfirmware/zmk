# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_KSCAN_GPIO_DRIVER kscan_gpio_matrix.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_KSCAN_GPIO_DRIVER kscan_gpio_direct.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_KSCAN_GPIO_DRIVER kscan_gpio_demux.c)