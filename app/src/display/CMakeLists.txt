
target_sources_ifdef(CONFIG_ZMK_DISPLAY app PRIVATE main.c)
target_sources_ifdef(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN app PRIVATE status_screen.c)

add_subdirectory(widgets/)