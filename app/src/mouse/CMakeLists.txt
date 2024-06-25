if(CONFIG_ZMK_MOUSE)
    target_sources(app PRIVATE hid.c)

    target_sources_ifdef(CONFIG_ZMK_USB app PRIVATE usb_hid.c)
    target_sources_ifdef(CONFIG_ZMK_BLE app PRIVATE hog.c)
endif()