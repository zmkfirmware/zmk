# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_include_directories(.)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MAX17048  max17048.c)
zephyr_library_sources_ifndef(CONFIG_MAX17048 ${ZEPHYR_BASE}/misc/empty_file.c)
