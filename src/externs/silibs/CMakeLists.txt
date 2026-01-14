# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/silibs)
set(LIB_OUTS_DIR ${CMAKE_BINARY_DIR}/externs/silibs)

include(FetchContent)
FetchContent_Declare(
    silibs
    GIT_REPOSITORY $ENV{REPO_APP_GIT_SILIBS.URL}
    GIT_TAG        $ENV{REPO_APP_GIT_SILIBS.TAG}
    SOURCE_DIR     ${TARGET_DIR}
)

FetchContent_GetProperties(silibs)

if (NOT silibs_POPULATED)
    FetchContent_Populate(silibs)
endif()

add_subdirectory(libraries)

file(COPY ${CMAKE_SOURCE_DIR}/.externs/repo/silibs/soc_fw_shared/include/cper_descriptors
     DESTINATION ${CMAKE_BINARY_DIR}/cper_descriptors)