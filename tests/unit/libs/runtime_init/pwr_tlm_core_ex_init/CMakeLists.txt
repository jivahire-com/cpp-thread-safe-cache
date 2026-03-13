# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET pwr_core_ex_init.test)

set(SOURCES
    test_pwr_core_ex_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/pwr_tlm_core_ex_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        ms::lib::boot_status
        ms::lib::arm_intrinsic
        ms::lib::pwr_tlm_core_ex
        ms::cmocka::testrunner
        silibs::idsw
        silibs::idsw_kng
        ms::lib::utils
)

# Defining platform APIs
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    boot_status_notify_extd
    idsw_get_cpu_type
    idsw_get_die_id
)
