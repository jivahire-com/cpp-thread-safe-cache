set(TARGET scp_error_domain.test)

set(SOURCES
    test_error_domain_register_scp.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_domain_scp.c
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_injection_scp.c
    # Include silibs_platform.c directly so mmio_read32/mmio_write32 calls from mmio_update32 can be wrapped
    ${CMAKE_SOURCE_DIR}/.externs/repo/cortex_m7/src/externs/silibs_common/silibs_platform/silibs_platform.c
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
        fwlib.lib.interface.platform.interrupts
        silibs::idsw_kng
        silibs::semaphore
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
        ms::cmsis::interface
)

# Get private includes for module under test
target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::lib::error_domain::scp,INCLUDE_DIRECTORIES>
        ./
    SYSTEM PRIVATE
        # Include dirs for silibs_platform.c that we compile directly
        ${CMAKE_SOURCE_DIR}/.externs/repo/silibs/silibs_platform/include
        ${CMAKE_SOURCE_DIR}/.externs/repo/cortex_m7/src/externs/silibs_common/silibs_platform/inc
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

# AP Watchdog ISR Test
set(AP_WDT_TARGET ap_wdt_isr.test)

set(AP_WDT_SOURCES
    test_error_domain_ap_wdt_isr.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_domain_ap_wdt.c
)

# Compile AP WDT ISR test
add_executable(${AP_WDT_TARGET} ${AP_WDT_SOURCES})

target_link_libraries(${AP_WDT_TARGET}
    PRIVATE
        fwlib.dfwk
        fwlib.lib.init
        fwlib.lib.interface.platform.dbgprint
        fwlib.lib.utils
        fwlib.platform.dfwkhost.threadx
        fwlib.lib.interface.platform.interrupts
        silibs::idsw_kng
        silibs::kng_silibs_platform
        silibs::idhw
        silibs::idsw
        ms::cmocka::testrunner
        ms::service::atu_service
        ms::service::health_monitor
        ms::lib::arm_intrinsic
        ms::lib::cortex_m7::nvic
        ms::lib::error_domain::scp
        ms::lib::crash_dump
        ms::lib::warm_start
)

# Get private includes for AP WDT test
target_include_directories(${AP_WDT_TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::lib::error_domain::scp,INCLUDE_DIRECTORIES>
        ./
)

# Generic ports implement within this project
set_target_properties(${AP_WDT_TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${AP_WDT_TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_compile_options(${AP_WDT_TARGET} PRIVATE -Wno-unused-function)

target_compile_definitions(
    ${AP_WDT_TARGET}
    PRIVATE
        SCP_RUNTIME_INIT
        SCP_CONFIG_VARIABLE_TYPE
)

lint_target(${AP_WDT_TARGET})
format_target(${AP_WDT_TARGET})
coverage_target(${AP_WDT_TARGET})
unit_test_target(${AP_WDT_TARGET})

# Mock the functions needed for AP WDT ISR test
mock_target(${AP_WDT_TARGET}
    atu_map
    atu_unmap
    crash_dump_bug_check
    crash_dump_context
    crash_dump_register_address32
    hm_submit_cper
    idsw_get_die_id
    mmio_read32
    nvic_get_current_irq
    nvic_irq_clear_pending
    ws_data_put
    CdRegisterCustomChunk
)
