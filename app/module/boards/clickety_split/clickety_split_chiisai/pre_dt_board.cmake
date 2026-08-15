# Copyright (c) 2025 The ZMK Contributors
# SPDX-License-Identifier: MIT

# Suppress duplicate unit-address warnings for overlapping Nordic peripherals.
# https://docs.zephyrproject.org/latest/build/dts/intro-input-output.html

list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
