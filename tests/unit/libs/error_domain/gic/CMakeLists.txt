# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET gic_error_domain.test)

set(SOURCES
    test_error_domain_gic.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_domain_gic.c
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_injection_gic.c
)

# Compile test
add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        silibs::idsw_kng
        silibs::semaphore
        silibs::kng_silibs_platform
        silibs::idhw
        silibs::idsw
        ms::cmocka::testrunner
        ms::service::atu_service
        ms::service::health_monitor
        ms::service::health_monitor::cli
        ms::service::power
        ms::lib::arm_intrinsic
        ms::lib::cortex_m7::nvic
        ms::lib::error_domain::scp
        ms::lib::crash_dump
        ms::lib::pex_rng
        ms::lib::scp_interrupts
)

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::lib::error_domain::scp,INCLUDE_DIRECTORIES>
        ./
)

# Generic ports implement within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_compile_options(gic_error_domain.test PRIVATE -Wno-unused-function)


lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    hm_register_error_domain 
    mmio_write64
    atu_translate_address
)
