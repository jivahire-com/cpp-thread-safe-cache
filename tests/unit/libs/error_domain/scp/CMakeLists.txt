set(TARGET scp_error_domain.test)

set(SOURCES
    test_error_domain_register_scp.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_domain_register_scp.c
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
        ms::lib::arm_intrinsic
        ms::lib::cortex_m7::nvic
        ms::lib::error_domain::scp
        ms::lib::crash_dump
        ms::lib::scp_interrupts
)

# Generic ports implement within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    crash_dump_bug_check
    hm_register_error_domain
    hm_submit_cper
    mmio_read32
    mmio_write32
    nvic_global_enable
    nvic_global_disable
    nvic_irq_set_isr
    nvic_irq_enable
)
