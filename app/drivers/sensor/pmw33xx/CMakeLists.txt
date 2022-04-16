# Copyright (c) 2020 The ZMK Contributors
# SPDX-License-Identifier: MIT

zephyr_include_directories(.)
zephyr_include_directories_ifdef(CONFIG_PMW33XX_3389 CONFIG_PMW33XX_3389_SROM_INCLUDE_DIR)

zephyr_library()

zephyr_library_sources(pmw33xx.c)
zephyr_library_sources_ifdef(CONFIG_PMW33XX_TRIGGER pmw33xx_trigger.c)
