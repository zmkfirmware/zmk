# Copyright (c) 2021 The ZMK Contributors
# SPDX-License-Identifier: MIT

board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--dfuse")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
