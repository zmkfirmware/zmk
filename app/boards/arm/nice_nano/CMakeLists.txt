set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/../tools/uf2/utils/uf2conv.py
  -b 0x26000
  -f NRF52
  -o ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.uf2
  ${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}
)
