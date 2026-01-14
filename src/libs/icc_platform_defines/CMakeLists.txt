# Copyright (c) Microsoft Corporation. All rights reserved.


set(TARGET icc_platform_defines)

add_library(${TARGET} INTERFACE)

target_link_libraries(${TARGET}
    INTERFACE
        silibs::sdm_command_desc
        fwlib.service.iccBase
)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(ms::lib::icc_platform_defines ALIAS ${TARGET})
