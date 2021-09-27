board_runner_args(pyocd "--target=nrf52840")
set(OPENOCD_NRF5_INTERFACE "cmsis-dap")
set(OPENOCD_NRF5_SUBFAMILY "nRF52840")
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-nrf5.board.cmake)
