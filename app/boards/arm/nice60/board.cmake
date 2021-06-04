# Copyright (c) 2021 Nick Winans
# SPDX-License-Identifier: MIT

set(OPENOCD_NRF5_SUBFAMILY nrf52)
board_runner_args(nrfjprog "--nrf-family=NRF52" "--softreset")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-nrf5.board.cmake)
