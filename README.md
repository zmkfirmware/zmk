https://zmk.dev/docs/development/setup/docker

https://docs.docker.com/engine/install/linux-postinstall/

https://zmk.dev/docs/development/build-flash

https://www.reddit.com/r/ErgoMechKeyboards/comments/15t3o6k/custom_art_on_niceview_displays/

/zmk/app/boards/shields/nice_view

make sure the right zmk-configs volume is mounted before building

west build -d build/right -p -b "nice_nano_v2" -- -DZMK_CONFIG="../../zmk-config/config" -DSHIELD="corne_right nice_view_adapter nice_view"

west build -d build/left -p -b "nice_nano_v2" -- -DZMK_CONFIG="../../zmk-config/config" -DSHIELD="corne_left nice_view_adapter nice_view"
