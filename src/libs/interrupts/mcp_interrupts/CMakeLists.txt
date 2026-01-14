# Copyright (c) Microsoft Corporation. All rights reserved.


set(TARGET mcp_interrupts)

add_library(${TARGET} INTERFACE)

target_include_directories(${TARGET} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_link_libraries(${TARGET} INTERFACE silibs::kng_silibs_platform)

add_library(ms::lib::mcp_interrupts ALIAS ${TARGET})
