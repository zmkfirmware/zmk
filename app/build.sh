#!/usr/bin/env bash

pristine=""
if [ $# -gt 0 ] && [ "$1" = "-p" -o "$1" = "--pristine" ]; then
	pristine="--pristine"
fi

COMMON_ARGS=(
	"-DZEPHYR_TOOLCHAIN_VARIANT=zephyr"
	"-DZEPHYR_SDK_INSTALL_DIR=/opt/pacman/opt/zephyr-sdk"
	"-DZMK_CONFIG=$(pwd)/../../zmk-config/config"
	"-Wno-dev"
)

set -x

west build -d build/left -b mikoto_520 ${pristine} -- -DSHIELD=misaka_left "${COMMON_ARGS[@]}"   && mv build/left/zephyr/zmk.uf2 ./left.uf2
west build -d build/right -b mikoto_520 ${pristine} -- -DSHIELD=misaka_right "${COMMON_ARGS[@]}" && mv build/right/zephyr/zmk.uf2 ./right.uf2


