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

    # RTOSMON Configuration for ThreadX Ingestion
    set(RTOSMON_DIR ${REPO_ROOT_DIR}/.externs/repo/1pfw_fwlibs/src/libs/rtosmon)
    set(TX_USER_DIR ${CMAKE_SOURCE_DIR}/.externs/repo/cortex_m7/src/externs/threadx/threadx_lib/cortex_m7/inc)
    set(TX_USER_FILE ${TX_USER_DIR}/tx_user.h)
    set(RTOSMON_SRCS
        ${RTOSMON_DIR}/src/tx_execution_profile.c
        ${RTOSMON_DIR}/src/tx_el.c
    )
    set(RTOSMON_INC_DIR
        ${RTOSMON_DIR}/inc
    )
    set(RTOSMON_COMPILE_DEFINITIONS
        TX_INCLUDE_USER_DEFINE_FILE
        TX_ENABLE_EXECUTION_CHANGE_NOTIFY
        TX_EXECUTION_PROFILE_ENABLE
        TX_ENABLE_EVENT_TRACE
        TX_CORTEX_M_EPK
    )
    set(RTOSMON_COMPILE_OPTIONS
        $<$<COMPILE_LANGUAGE:ASM>:-DTX_EXECUTION_PROFILE_ENABLE>
        $<$<COMPILE_LANGUAGE:ASM>:-DTX_ENABLE_EXECUTION_CHANGE_NOTIFY>
        $<$<COMPILE_LANGUAGE:ASM>:-DTX_ENABLE_EVENT_TRACE>
    )
    set(RTOSMON_LIBS
        fwlib.lib.rtosmon
        fwlib.lib.status
        ms::lib::utils
    )

    # Add the source directory
    add_subdirectory(${TARGET_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src/cortex_m7 EXCLUDE_FROM_ALL)
    # Add the test utilities directory
    add_subdirectory(${TARGET_DIR}/tests/utilities ${CMAKE_CURRENT_BINARY_DIR}/utlities/cortex_m7 EXCLUDE_FROM_ALL)
    # Add the rtosmon platform port
endif()

include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/lint.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/format.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/coverage.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/mock.cmake)
