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
        list(APPEND BOARD_ROOT ${ZMK_CONFIG})
    endif()
    if(EXISTS ${ZMK_CONFIG}/dts)
        message(STATUS "Adding ZMK config directory as DTS root: ${ZMK_CONFIG}")
        list(APPEND DTS_ROOT ${ZMK_CONFIG})
    endif()
endif()


if(DEFINED SHIELD)
    string(REPLACE " " ";" SHIELD_AS_LIST "${SHIELD}")
endif()

string(FIND "${BOARD}" "@" REVISION_SEPARATOR_INDEX)
if(NOT (REVISION_SEPARATOR_INDEX EQUAL -1))
    math(EXPR BOARD_REVISION_INDEX "${REVISION_SEPARATOR_INDEX} + 1")
    string(SUBSTRING ${BOARD} ${BOARD_REVISION_INDEX} -1 BOARD_REVISION)
    string(SUBSTRING ${BOARD} 0 ${REVISION_SEPARATOR_INDEX} BOARD)
endif()

foreach(root ${BOARD_ROOT})
    set(shield_dir ${root}/boards/shields)
    # Match the Kconfig.shield files in the shield directories to make sure we are
    # finding shields, e.g. x_nucleo_iks01a1/Kconfig.shield
    file(GLOB_RECURSE shields_refs_list ${shield_dir}/*/Kconfig.shield)
    unset(SHIELD_LIST)
    foreach(shields_refs ${shields_refs_list})
        get_filename_component(shield_path ${shields_refs} DIRECTORY)
        file(GLOB shield_overlays RELATIVE ${shield_path} ${shield_path}/*.overlay)
        foreach(overlay ${shield_overlays})
            get_filename_component(shield ${overlay} NAME_WE)
            list(APPEND SHIELD_LIST ${shield})
            set(SHIELD_DIR_${shield} ${shield_path})
        endforeach()
    endforeach()

    if (EXISTS "${root}/boards/${BOARD}.overlay")
        list(APPEND shield_dts_files "${root}/boards/${BOARD}.overlay")
    endif()
    if (NOT DEFINED BOARD_DIR_NAME)
        find_path(BOARD_DIR
            NAMES ${BOARD}_defconfig
            PATHS ${root}/boards/*/*
            NO_DEFAULT_PATH
            )
        if(BOARD_DIR)
            get_filename_component(BOARD_DIR_NAME ${BOARD_DIR} NAME)
            list(APPEND KEYMAP_DIRS ${BOARD_DIR})
        endif()
    endif()

    if(DEFINED SHIELD)
        foreach(s ${SHIELD_AS_LIST})
            if(NOT ${s} IN_LIST SHIELD_LIST)
                continue()
            endif()
            message(STATUS "Adding ${SHIELD_DIR_${s}}")
            list(APPEND KEYMAP_DIRS ${SHIELD_DIR_${s}})
            get_filename_component(shield_dir_name ${SHIELD_DIR_${s}} NAME)
            list(APPEND SHIELD_DIR ${shield_dir_name})
        endforeach()
    endif()
endforeach()

if(EXISTS ${BOARD_DIR}/revision.cmake)
    # Board provides revision handling.
    include(${BOARD_DIR}/revision.cmake)
elseif(BOARD_REVISION)
    message(WARNING "Board revision ${BOARD_REVISION} specified for ${BOARD}, \
                     but board has no revision so revision will be ignored.")
endif()

if(DEFINED BOARD_REVISION)
    string(REPLACE "." "_" BOARD_REVISION_STRING ${BOARD_REVISION})
    set(KEYMAP_BOARD_REVISION_PREFIX "${BOARD}_${BOARD_REVISION_STRING}")
else()
    set(KEYMAP_BOARD_REVISION_PREFIX "")
endif()

# Give a shield like `kyria_rev2_left` we want to use `kyria_rev2` and `kyria` as candidate names for
# overlay/conf/keymap files.
if(DEFINED SHIELD)
    foreach(s ${SHIELD_AS_LIST})
        if (DEFINED $SHIELD_DIR_${s})
            get_filename_component(shield_dir_name ${SHIELD_DIR_${s}} NAME)
        endif()
        string(REPLACE "_" ";" S_PIECES ${s})
        list(LENGTH S_PIECES S_PIECES_LEN)
        while(NOT S_PIECES STREQUAL "")
            list(POP_BACK S_PIECES)
            list(JOIN S_PIECES "_" s_substr)
            if ("${s_substr}" STREQUAL "" OR "${s_substr}" STREQUAL "${shield_dir_name}")
                break()
            endif()
            list(APPEND shield_candidate_names ${s_substr})
        endwhile()
    endforeach()
endif()

if (ZMK_CONFIG)
    if (EXISTS ${ZMK_CONFIG})
        message(STATUS "ZMK Config directory: ${ZMK_CONFIG}")
        list(PREPEND KEYMAP_DIRS "${ZMK_CONFIG}")

        if (DEFINED SHIELD)
            foreach (s ${shield_candidate_names} ${SHIELD_AS_LIST})
                if (DEFINED ${SHIELD_DIR_${s}})
                    get_filename_component(shield_dir_name ${SHIELD_DIR_${s}} NAME)
                endif()
                list(APPEND overlay_candidates "${ZMK_CONFIG}/${s}_${BOARD}.overlay")
                list(APPEND overlay_candidates "${ZMK_CONFIG}/${s}.overlay")
                if (NOT "${shield_dir_name}" STREQUAL "${s}")
                    list(APPEND config_candidates "${ZMK_CONFIG}/${shield_dir_name}_${BOARD}.conf")
                    list(APPEND config_candidates "${ZMK_CONFIG}/${shield_dir_name}.conf")
                endif()
                list(APPEND config_candidates "${ZMK_CONFIG}/${s}_${BOARD}.conf")
                list(APPEND config_candidates "${ZMK_CONFIG}/${s}.conf")
            endforeach()
        endif()

        # TODO: Board revisions?
        list(APPEND overlay_candidates "${ZMK_CONFIG}/${BOARD_DIR_NAME}.overlay")
        list(APPEND overlay_candidates "${ZMK_CONFIG}/${BOARD}.overlay")
        list(APPEND overlay_candidates "${ZMK_CONFIG}/default.overlay")
        list(APPEND config_candidates "${ZMK_CONFIG}/${BOARD_DIR_NAME}.conf")
        list(APPEND config_candidates "${ZMK_CONFIG}/${BOARD}.conf")
        list(APPEND config_candidates "${ZMK_CONFIG}/default.conf")

        foreach(overlay ${overlay_candidates})
            if (EXISTS "${overlay}")
                message(STATUS "ZMK Config devicetree overlay: ${overlay}")
                list(APPEND shield_dts_files "${overlay}")
                break()
            endif()
        endforeach()

        foreach(conf ${config_candidates})
            if (EXISTS "${conf}")
                message(STATUS "ZMK Config Kconfig: ${conf}")
                list(APPEND shield_conf_files "${conf}")
            endif()
        endforeach()
    else()
        message(WARNING "Unable to locate ZMK config at: ${ZMK_CONFIG}")
    endif()
endif()


if(NOT KEYMAP_FILE)
    foreach(keymap_dir ${KEYMAP_DIRS})
        foreach(keymap_prefix ${shield_candidate_names} ${SHIELD_AS_LIST} ${SHIELD_DIR} ${KEYMAP_BOARD_REVISION_PREFIX} ${BOARD} ${BOARD_DIR_NAME})
            if (EXISTS ${keymap_dir}/${keymap_prefix}.keymap)
                set(KEYMAP_FILE "${keymap_dir}/${keymap_prefix}.keymap" CACHE STRING "Selected keymap file")
                message(STATUS "Using keymap file: ${KEYMAP_FILE}")
                set(DTC_OVERLAY_FILE ${KEYMAP_FILE})
                break()
            endif()
        endforeach()
    endforeach()
else()
    message(STATUS "Using keymap file: ${KEYMAP_FILE}")
    set(DTC_OVERLAY_FILE ${KEYMAP_FILE})
endif()

if (NOT KEYMAP_FILE)
    message(WARNING "Failed to locate keymap file!")
endif()
