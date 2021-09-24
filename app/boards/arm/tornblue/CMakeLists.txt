# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/../tools/uf2/utils/uf2conv.py
  -c
  -b 0x26000
  -f 0xADA52840
  -o ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.uf2
  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
)

zephyr_library()
zephyr_library_sources(led_driver.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)
zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
