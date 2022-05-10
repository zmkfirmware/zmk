# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

add_subdirectory_ifdef(CONFIG_GPIO_MCP23017 mcp23017)
add_subdirectory_ifdef(CONFIG_GPIO_MAX7318 max7318)
