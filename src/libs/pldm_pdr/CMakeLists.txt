# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET pldm_pdr)

# Generate time of the PDR
string(TIMESTAMP BUILD_SECONDS "%S" UTC)
math(EXPR BUILD_SECONDS ${BUILD_SECONDS})
string(TIMESTAMP BUILD_MINUTES "%M" UTC)
math(EXPR BUILD_MINUTES ${BUILD_MINUTES})
string(TIMESTAMP BUILD_HOURS "%H" UTC)
math(EXPR BUILD_HOURS ${BUILD_HOURS})
string(TIMESTAMP BUILD_DAY "%d" UTC)
math(EXPR BUILD_DAY ${BUILD_DAY})
string(TIMESTAMP BUILD_MONTH "%m" UTC)
math(EXPR BUILD_MONTH ${BUILD_MONTH})
string(TIMESTAMP BUILD_YEAR "%Y" UTC)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/pldm_pdr_time.c.in
    ${CMAKE_CURRENT_BINARY_DIR}/pldm_pdr_time.c
    @ONLY
)
set (SRC
    ${CMAKE_CURRENT_BINARY_DIR}/pldm_pdr_time.c
)
add_library(${TARGET} STATIC ${SRC})
target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.pldm.protocol
        extern.libpldm
)

add_library(ms::lib::pldm_pdr ALIAS ${TARGET})