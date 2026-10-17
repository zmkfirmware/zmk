# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

# Suppress warnings caused by Nordic SoC nodes that intentionally share unit addresses.
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
