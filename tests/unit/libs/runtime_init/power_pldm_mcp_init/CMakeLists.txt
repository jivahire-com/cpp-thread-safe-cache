# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET power_pldm_mcp_init.test)

set(SOURCES
    test_power_pldm_mcp_init.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/runtime_init/power_pldm_mcp_init.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.lib.init
        fwlib.lib.utils
        fwlib.lib.interface.platform.dbgprint
        fwlib.service.iccBase
        fwlib.service.pldm
        silibs::idhw
        silibs::idsw
        silibs::idsw_kng
        ms::cmocka::testrunner
        ms::service::pwr_pldm_mcp
        silibs_platform
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
    fpfw_init_get_handle
    idsw_get_cpu_type
    idsw_get_die_id
    fpfw_pldm_service_register_numeric_effecter
    fpfw_pldm_service_register_state_sensor
    FpFwLockInitialize
)
