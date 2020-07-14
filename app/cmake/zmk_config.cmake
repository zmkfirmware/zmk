# TODO: Check for env or command line "ZMK_CONFIG" setting.
#  * That directory should load
#    * defconfigs,
#    * .conf file,
#    * single overlay,
#    * or per board/shield.

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

foreach(root ${BOARD_ROOT})
	if (EXISTS "${root}/boards/${BOARD}.overlay")
		list(APPEND ZMK_DTC_FILES "${root}/boards/${BOARD}.overlay")
	endif()
	find_path(BOARD_DIR
	    NAMES ${BOARD}_defconfig
	    PATHS ${root}/boards/*/*
	    NO_DEFAULT_PATH
	    )
    	if(BOARD_DIR)
		if (EXISTS "${BOARD_DIR}/keymap")
			list(APPEND KEYMAP_DIRS ${BOARD_DIR}/keymap)
		endif()
	endif()

	if(DEFINED SHIELD)
		find_path(shields_refs_list
		    NAMES ${SHIELD}.overlay
		    PATHS ${root}/boards/shields/*
		    NO_DEFAULT_PATH
		    )
		foreach(shield_path ${shields_refs_list})
			if (EXISTS "${shield_path}/keymap")
				list(APPEND KEYMAP_DIRS ${shield_path}/keymap)
			endif()
		endforeach()
	endif()
endforeach()

find_path(BASE_KEYMAPS_DIR
	NAMES ${KEYMAP}/keymap.overlay
	PATHS ${KEYMAP_DIRS}
	NO_DEFAULT_PATH
)

if (BASE_KEYMAPS_DIR)
	set(KEYMAP_DIR "${BASE_KEYMAPS_DIR}/${KEYMAP}" CACHE STRING "Selected keymap directory")
	message(STATUS "Keyboard Base Keymap: ${KEYMAP_DIR}/")
	# Used to let local imports of custom keycodes work as expected
	list(APPEND DTS_ROOT ${KEYMAP_DIR})
	if (EXISTS "${KEYMAP_DIR}/include")
		include_directories("${KEYMAP_DIR}/include")
	endif()
	list(APPEND ZMK_DTC_FILES "${KEYMAP_DIR}keymap.overlay")
endif()

if (ZMK_CONFIG)
	if (EXISTS "${ZMK_CONFIG}")
		message(STATUS "ZMK Config directory: ${ZMK_CONFIG}")
		list(APPEND DTS_ROOT ${ZMK_CONFIG})
		if (EXISTS "${ZMK_CONFIG}/include")
			include_directories("${ZMK_CONFIG}/include")
		endif()

		if (SHIELD)
			list(APPEND overlay_candidates "${ZMK_CONFIG}/${SHIELD}_${BOARD}.overlay")
			list(APPEND overlay_candidates "${ZMK_CONFIG}/${SHIELD}.overlay")
			list(APPEND config_candidates "${ZMK_CONFIG}/${SHIELD}_${BOARD}.conf")
			list(APPEND config_candidates "${ZMK_CONFIG}/${SHIELD}.conf")
		endif()

		# TODO: Board revisions?
		list(APPEND overlay_candidates "${ZMK_CONFIG}/${BOARD}.overlay")
		list(APPEND overlay_candidates "${ZMK_CONFIG}/default.overlay")
		list(APPEND config_candidates "${ZMK_CONFIG}/${BOARD}.conf")
		list(APPEND config_candidates "${ZMK_CONFIG}/default.conf")

		foreach(overlay ${overlay_candidates})
			if (EXISTS "${overlay}")
				message(STATUS "ZMK Config devicetree overlay: ${overlay}")
				list(APPEND ZMK_DTC_FILES "${overlay}")
				break()
			endif()
		endforeach()

		foreach(conf ${config_candidates})
			if (EXISTS "${conf}")
				message(STATUS "ZMK Config Kconfig: ${overlay}")
				set(CONF_FILE "${conf}")
				break()
			endif()
		endforeach()
	else()
		message(WARNING "Unable to locate ZMK config at: ${ZMK_CONFIG}")
	endif()
endif()

if (ZMK_DTC_FILES)
	string(REPLACE ";" " " DTC_OVERLAY_FILE "${ZMK_DTC_FILES}")
endif()
