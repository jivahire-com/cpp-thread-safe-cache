# Copyright (c) Microsoft Corporation. All rights reserved.

# This make is to clone the zlib library based on input provided in packages.xml file
# Post cloning the library, the make file also describes the files needed to build the library 
# and exports zlib as library to be used

set(TARGET extern.populate.zlib)
set(TARGET_DIR ${REPO_ROOT_DIR}/.externs/repo/zlib)

include(FetchContent)
FetchContent_Declare(
    ${TARGET}
    GIT_REPOSITORY $ENV{REPO_APP_GIT_ZLIB.URL}
    GIT_TAG        $ENV{REPO_APP_GIT_ZLIB.TAG}
    SOURCE_DIR     ${TARGET_DIR}
)

FetchContent_GetProperties(${TARGET})

if (NOT ${TARGET}_POPULATED)
    FetchContent_Populate(${TARGET})
    
    # Add the source directory
    add_subdirectory(${TARGET_DIR} ${CMAKE_CURRENT_BINARY_DIR}/zlib EXCLUDE_FROM_ALL)
endif()

#Set sources in zlib to be compiled 
SET(SRC
    ${TARGET_DIR}/adler32.c
    ${TARGET_DIR}/compress.c
    ${TARGET_DIR}/crc32.c
    ${TARGET_DIR}/deflate.c
    ${TARGET_DIR}/inflate.c
    ${TARGET_DIR}/infback.c
    ${TARGET_DIR}/inftrees.c
    ${TARGET_DIR}/inffast.c
    ${TARGET_DIR}/trees.c
    ${TARGET_DIR}/uncompr.c
    ${TARGET_DIR}/zutil.c
)

add_library(${TARGET} STATIC ${SRC})
target_include_directories(
    ${TARGET}
      PUBLIC
            ${TARGET_DIR}
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../src/externs/zlib/zlib
)

# Disable HAVE_MEMCPY to force byte-by-byte copying for alignment safety
# This avoids Usage Fault when strict alignment is enforced
target_compile_definitions(${TARGET} PRIVATE NO_MEMCPY)

add_library(ms::lib::zlib ALIAS extern.populate.zlib)
