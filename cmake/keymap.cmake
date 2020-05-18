
get_property(cached_keymap_value CACHE KEYMAP PROPERTY VALUE)

# There are actually 4 sources, the three user input sources, and the
# previously used value (CACHED_KEYMAP). The previously used value has
# precedence, and if we detect that the user is trying to change the
# value we give him a warning about needing to clean the build
# directory to be able to change keymaps.

set(keymap_cli_argument ${cached_keymap_value}) # Either new or old
if(keymap_cli_argument STREQUAL CACHED_KEYMAP)
	# We already have a CACHED_KEYMAP so there is no new input on the CLI
  unset(keymap_cli_argument)
endif()

set(keymap_app_cmake_lists ${KEYMAP})
if(cached_keymap_value STREQUAL KEYMAP)
	# The app build scripts did not set a default, The KEYMAP we are
  # reading is the cached value from the CLI
  unset(keymap_app_cmake_lists)
endif()

if(CACHED_KEYMAP)
  # Warn the user if it looks like he is trying to change the keymap
  # without cleaning first
  if(keymap_cli_argument)
	  if(NOT (CACHED_KEYMAP STREQUAL keymap_cli_argument))
      message(WARNING "The build directory must be cleaned pristinely when changing keymaps")
      # TODO: Support changing keymaps without requiring a clean build
    endif()
  endif()

  set(KEYMAP ${CACHED_KEYMAP})
elseif(keymap_cli_argument)
	set(KEYMAP ${keymap_cli_argument})

elseif(DEFINED ENV{KEYMAP})
	set(KEYMAP $ENV{KEYMAP})

elseif(keymap_app_cmake_lists)
	set(KEYMAP ${keymap_app_cmake_lists})

else()
	set(KEYMAP default)
	message(STATUS "KEYMAP defaulted to 'default'")
endif()

message(STATUS "Keymap: ${KEYMAP}")

# Store the selected keymap in the cache
set(CACHED_KEYMAP ${KEYMAP} CACHE STRING "Selected keymap")

set(ZMK_APP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND KEYMAP_DIRS ${ZMK_APP_DIR}/keymaps)

foreach(root ${BOARD_ROOT})
	find_path(BOARD_DIR
	    NAMES ${BOARD}_defconfig
	    PATHS ${root}/boards/*/*
	    NO_DEFAULT_PATH
	    )
    	if(BOARD_DIR)
		list(APPEND KEYMAP_DIRS ${BOARD_DIR}/keymaps)
	endif()

	if(DEFINED SHIELD)
		find_path(shields_refs_list
		    NAMES ${SHIELD}.overlay
		    PATHS ${root}/boards/shields/*
		    NO_DEFAULT_PATH
		    )
		foreach(shield_path ${shields_refs_list})
			list(APPEND KEYMAP_DIRS ${shield_path}/keymaps)
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
	message(STATUS "Using keymap directory: ${KEYMAP_DIR}/")
	# Used to let local imports of custom keycodes work as expected
	list(APPEND DTS_ROOT ${KEYMAP_DIR})
	if (EXISTS "${KEYMAP_DIR}/include")
		include_directories("${KEYMAP_DIR}/include")
	endif()
	set(DTC_OVERLAY_FILE ${KEYMAP_DIR}/keymap.overlay)
endif()
