# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET wdog_service.test)

set(SOURCES
    wdog_service_tests.cpp
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_include_directories(
    ${TARGET}
        PRIVATE
            ${REPO_ROOT_DIR}/src/services/wdog_service
)

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.service.pldm
        ms::cmocka::testrunner
        ms::lib::threadx
        silibs::idsw
        silibs::idsw_kng
    
        ms::lib::utils
        ms::lib::error_codes
        ms::service::wdog
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
    _txe_thread_create
    _tx_thread_sleep
    wdog_cmsdk_apb_init
    wdog_cmsdk_apb_reload
)
