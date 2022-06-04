# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_library_named(zmk__drivers__gpio)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)

zephyr_library_sources_ifdef(CONFIG_GPIO_595 gpio_595.c)
zephyr_library_sources_ifdef(CONFIG_GPIO_MCP23017 gpio_mcp23017.c)
zephyr_library_sources_ifdef(CONFIG_GPIO_MAX7318 gpio_max7318.c)
