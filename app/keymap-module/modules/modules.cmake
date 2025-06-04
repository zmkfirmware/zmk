# TODO: Check for env or command line "ZMK_CONFIG" setting.
#  * That directory should load
#    * defconfigs,
#    * .conf file,
#    * single overlay,
#    * or per board/shield.

list(APPEND BOARD_ROOT ${APPLICATION_SOURCE_DIR})
list(APPEND DTS_ROOT ${APPLICATION_SOURCE_DIR})

get_property(cached_user_config_value CACHE ZMK_CONFIG PROPERTY VALUE)

set(user_config_cli_argument ${cached_user_config_value}) # Either new or old
if(user_config_cli_argument STREQUAL CACHED_ZMK_CONFIG)
    # We already have a CACHED_ZMK_CONFIG so there is no new input on the CLI
  unset(user_config_cli_argument)
endif()

set(user_config_app_cmake_lists ${ZMK_CONFIG})
if(cached_user_config_value STREQUAL ZMK_CONFIG)
    # The app build scripts did not set a default, The ZMK_CONFIG we are
  # reading is the cached value from the CLI
  unset(user_config_app_cmake_lists)
endif()

if(CACHED_ZMK_CONFIG)
  # Warn the user if it looks like he is trying to change the user_config
  # without cleaning first
  if(user_config_cli_argument)
      if(NOT (CACHED_ZMK_CONFIG STREQUAL user_config_cli_argument))
      message(WARNING "The build directory must be cleaned pristinely when changing user ZMK config")
    endif()
  endif()

  set(ZMK_CONFIG ${CACHED_ZMK_CONFIG})
elseif(user_config_cli_argument)
    set(ZMK_CONFIG ${user_config_cli_argument})

elseif(DEFINED ENV{ZMK_CONFIG})
    set(ZMK_CONFIG $ENV{ZMK_CONFIG})

elseif(user_config_app_cmake_lists)
    set(ZMK_CONFIG ${user_config_app_cmake_lists})
endif()

# Store the selected user_config in the cache
set(CACHED_ZMK_CONFIG ${ZMK_CONFIG} CACHE STRING "Selected user ZMK config")

if (ZMK_CONFIG)
    set(ENV{ZMK_CONFIG} "${ZMK_CONFIG}")
    if(EXISTS ${ZMK_CONFIG}/boards)
        message(STATUS "Adding ZMK config directory as board root: ${ZMK_CONFIG}")
        message(DEPRECATION "The `config/boards` folder is deprecated. Please use a module instead. See https://zmk.dev/docs/development/hardware-integration/new-shield and https://zmk.dev/docs/development/module-creation for more information.")
        list(APPEND BOARD_ROOT ${ZMK_CONFIG})
    endif()
    if(EXISTS ${ZMK_CONFIG}/dts)
        message(STATUS "Adding ZMK config directory as DTS root: ${ZMK_CONFIG}")
        message(DEPRECATION "The `config/dts` folder is deprecated. Please use a module instead. See https://zmk.dev/docs/development/hardware-integration/new-shield and https://zmk.dev/docs/development/module-creation for more information.")
        list(APPEND DTS_ROOT ${ZMK_CONFIG})
    endif()
endif()
