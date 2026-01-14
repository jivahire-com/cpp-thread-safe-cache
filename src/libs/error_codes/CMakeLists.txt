# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng.lib.error_codes)
set(ERR_GEN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/kingsgate_error_codes/KNGError.json)
set(ERR_GEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(ERR_GEN_OUTPUT_PATH ${ERR_GEN_OUTPUT_DIR}/kng_error.h)
set(ERR_GEN_COMMAND err_gen -j ${ERR_GEN_SRC} -f ${ERR_GEN_OUTPUT_PATH})

add_custom_command(
    OUTPUT ${ERR_GEN_OUTPUT_PATH}
    COMMAND ${ERR_GEN_COMMAND}
    DEPENDS  ${ERR_GEN_SRC}
    COMMENT "Generating kng_error.h and writing to ${ERR_GEN_OUTPUT_PATH}"
)

add_custom_target(
    KNG_ERR_FILE 
    DEPENDS ${ERR_GEN_OUTPUT_PATH}
)

add_library(${TARGET} INTERFACE)
add_dependencies(${TARGET} KNG_ERR_FILE)
target_include_directories(${TARGET} INTERFACE ${ERR_GEN_OUTPUT_DIR})
add_library(ms::lib::error_codes ALIAS ${TARGET})
