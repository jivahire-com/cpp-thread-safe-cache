# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET extern.populate.cortex_m7)
set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/cortex_m7)

include(FetchContent)
FetchContent_Declare(
    ${TARGET}
    GIT_REPOSITORY $ENV{REPO_APP_GIT_CORTEX_M7.URL}
    GIT_TAG        $ENV{REPO_APP_GIT_CORTEX_M7.TAG}
    SOURCE_DIR     ${TARGET_DIR}
)

FetchContent_GetProperties(${TARGET})

if (NOT ${TARGET}_POPULATED)
    FetchContent_Populate(${TARGET})
    set(SILIBS_COMMON_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs/libraries/common)

    # rtosmon library settings
    set(1PFWLIB_RTOSMON_SRC
        ${REPO_ROOT_DIR}/.externs/repo/1pfw_fwlibs/src/libs/rtosmon/src/tx_execution_profile.c
        ${REPO_ROOT_DIR}/.externs/repo/1pfw_fwlibs/src/libs/rtosmon/src/tx_el.c)
    set(1PFWLIB_RTOSMON_INC
        ${REPO_ROOT_DIR}/.externs/repo/1pfw_fwlibs/src/libs/rtosmon/inc
        ${REPO_ROOT_DIR}/.externs/repo/1pfw_fwlibs/src/libs/Status/inc)
    set(1PFWLIB_RTOS_LIB
        fwlib.lib.rtosmon)
    set(1PFWLIB_RTOS_COMPILE_DEFINITIONS
        TX_ENABLE_EVENT_LOGGING
        TX_ENABLE_EXECUTION_CHANGE_NOTIFY)

    # Apply compile options globally since FetchContent target isn't a build target
    # This will enable TX_ENABLE_EXECUTION_CHANGE_NOTIFY in .S files of threadx as well
    add_compile_options(-O2 $<$<COMPILE_LANGUAGE:ASM>:-DTX_ENABLE_EXECUTION_CHANGE_NOTIFY>)

    # Add the source directory
    add_subdirectory(${TARGET_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src/cortex_m7 EXCLUDE_FROM_ALL)
    # Add the test utlities directory
    add_subdirectory(${TARGET_DIR}/tests/utilities ${CMAKE_CURRENT_BINARY_DIR}/utlities/cortex_m7 EXCLUDE_FROM_ALL)
    # Add the rtosmon platform port
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/rtosmon)

include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/lint.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/format.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/coverage.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/mock.cmake)
