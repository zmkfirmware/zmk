# SPDX-License-Identifier: MIT

list(APPEND EXTRA_DTC_FLAGS "-qq")

if(CONFIG_PINMUX)
zephyr_library()
zephyr_library_sources(pinmux.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
endif()
