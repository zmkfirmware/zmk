# Copyright (c) 2020-2021 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_include_directories(.)

zephyr_library()

zephyr_library_sources(battery_common.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_BATTERY_NRF_VDDH battery_nrf_vddh.c)
zephyr_library_sources_ifdef(CONFIG_ZMK_BATTERY_VOLTAGE_DIVIDER battery_voltage_divider.c)