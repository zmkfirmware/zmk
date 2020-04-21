# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr)

get_filename_component(ZEPHYR_RUST ${CMAKE_CURRENT_SOURCE_DIR}/zephyr-rust ABSOLUTE)
list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_RUST})

project(zmk)

# Add your source file to the "app" target. This must come after
# find_package(Zephyr) which defines the target.
target_sources(app PRIVATE src/main.c)

include(ExternalProject)

# Add rust_example as a CMake target
ExternalProject_Add(
    zmk_crate
    DOWNLOAD_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build --target thumbv7m-none-eabi COMMAND cargo build --release --target thumbv7m-none-eabi
    BINARY_DIR "${CMAKE_SOURCE_DIR}/zmk"
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${CMAKE_SOURCE_DIR}/zmk/target/thumbv7m-none-eabi/release/libzmk.a"
    LOG_BUILD ON)

# Create a wrapper CMake library that our app can link with
add_library(zmk_lib STATIC IMPORTED GLOBAL)
add_dependencies(
  zmk_lib
  zmk_crate
  )

set_target_properties(zmk_lib PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/zmk/target/thumbv7m-none-eabi/release/libzmk.a)
# target_link_libraries(zmk_lib
# debug "${CMAKE_SOURCE_DIR}/target/debug/zmk.a"
# optimized "${CMAKE_SOURCE_DIR}/target/release/zmk.a")

target_link_libraries(app PUBLIC zmk_lib)

