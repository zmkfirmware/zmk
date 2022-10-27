zephyr_library()
zephyr_library_sources(custom_runtime_code.c)
zephyr_library_include_directories(${ZEPHYR_BASE}/drivers)