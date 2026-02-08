# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET kng_mscp_unpack_image.test)

set(SOURCES
    test_unpack_image.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/boot_loader/unpack_image.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

set(CMAKE_INCLUDE_CURRENT_DIR ON)

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_link_libraries(${TARGET}
    PRIVATE
    ms::cmocka::testrunner
    ms::boot_loader::interface
    ms::lib::zlib
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})
