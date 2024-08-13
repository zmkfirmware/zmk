# Copyright (c) 2023 The ZMK Contributors
# SPDX-License-Identifier: MIT

target_sources_ifdef(CONFIG_SETTINGS_NONE app PRIVATE reset_settings_none.c)
target_sources_ifdef(CONFIG_SETTINGS_FCB app PRIVATE reset_settings_fcb.c)
target_sources_ifdef(CONFIG_SETTINGS_FILE app PRIVATE reset_settings_file.c)
target_sources_ifdef(CONFIG_SETTINGS_NVS app PRIVATE reset_settings_nvs.c)

target_sources_ifdef(CONFIG_ZMK_SETTINGS_RESET_ON_START app PRIVATE reset_settings_on_start.c)
