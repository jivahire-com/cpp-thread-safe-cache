# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET smc_tlm_mcp.test)

set(SOURCES
    smc_tlm_mcp_tests.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(
    ${TARGET}
        PRIVATE
            ${REPO_ROOT_DIR}/src/services/smc_tlm_mcp
            ${REPO_ROOT_DIR}/src/services/smc_tlm_mcp/inc
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        ms::cmocka::testrunner
        ms::service::smc_tlm_mcp
        silibs::atu
        silibs::idsw
        silibs::idsw_kng
        ms::lib::utils
        ms::lib::error_codes
        ms::lib::threadx
)

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
    idsw_get_die_id
    idsw_get_cpu_type
    atu_map
    mmio_read64
    mmio_write64
    _txe_timer_create
    EventWriteApSmcTelemetry
    bug_check_ex
)

target_compile_options(${TARGET} PRIVATE -Wno-address-of-temporary -Wno-c99-designator -Wno-missing-field-initializers)