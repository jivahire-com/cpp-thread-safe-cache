# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET power_telemetry_common_test.test)

add_executable(${TARGET}
    power_telemetry_common_test.cpp
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::cmocka::testrunner
        ms::lib::power_telemetry_common
        fwlib.lib.cfg_mgr
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

mock_target(${TARGET}
    config_get_power_r_loadline_vcpu0
    config_get_power_r_loadline_vcpu1
    config_get_power_vsys_r_loadline
)

set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")