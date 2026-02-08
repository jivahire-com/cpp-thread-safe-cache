# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET boot_status_init.test)

set(SOURCES
    test_boot_status_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/boot_status_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_compile_definitions(${TARGET} PRIVATE UNIT_TEST)

target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/libs/system_info/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        ms::lib::accelerator_ip
        ms::lib::boot_status
        ms::cmocka::testrunner
        silibs::idsw_kng
        silibs::platform
        silibs::sdm_command_desc
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    accel_is_isolation_enabled
    accel_setup_boot_status_code
    boot_status_init
    fpfw_init_get_handle
    idsw_get_cpu_type
)
