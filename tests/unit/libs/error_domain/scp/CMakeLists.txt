set(TARGET scp_error_domain.test)

set(SOURCES
    test_error_domain_register_scp.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_domain_scp.c
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_injection_scp.c
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
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_compile_options(scp_error_domain.test PRIVATE -Wno-unused-function)

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
    atu_find_map
    atu_map
    atu_unmap
    crash_dump_bug_check
    hm_register_error_domain
    hm_submit_cper
    idsw_get_die_id
    mmio_read32
    mmio_write32
    nvic_get_current_irq
    nvic_global_enable
    nvic_global_disable
    nvic_irq_set_isr
    nvic_irq_set_isr_with_param
    nvic_irq_enable
    nvic_irq_clear_pending
    wdog_cmsdk_apb_disable
    wdog_cmsdk_apb_init
    mmu_fault
    idhw_is_single_die_boot_en
    get_rng_config
    idsw_get_platform_sdv
    is_cached_space
    _txe_timer_create
    fpfw_init_get_handle
)
