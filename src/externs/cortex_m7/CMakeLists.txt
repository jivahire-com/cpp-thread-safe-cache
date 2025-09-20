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
    
    # Add the source directory
    add_subdirectory(${TARGET_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src/cortex_m7 EXCLUDE_FROM_ALL)
    # Add the test utlities directory
    add_subdirectory(${TARGET_DIR}/tests/utilities ${CMAKE_CURRENT_BINARY_DIR}/utlities/cortex_m7 EXCLUDE_FROM_ALL)
endif()

include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/lint.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/format.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/coverage.cmake)
include($ENV{REPO_APP_CMAKE_TOOLCHAIN_DIR}/cmake/mock.cmake)
