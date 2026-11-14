zephyr_include_directories(include)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)

add_subdirectory(drivers)
add_subdirectory(test-behaviors)
add_subdirectory(lib)
