# Copyright (c) 2022 The ZMK Contributors
# SPDX-License-Identifier: MIT
target_link_libraries(app PRIVATE
    COBS
)

if (CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE central.c)
  target_sources(app PRIVATE interrupt.c)
else ()
  target_sources(app PRIVATE peripheral.c)
  target_sources(app PRIVATE polling.c)
endif()
