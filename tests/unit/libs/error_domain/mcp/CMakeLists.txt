# Copyright (c) Microsoft Corporation. All rights reserved.

set(TARGET mcp_error_domain.test)

set(SOURCES
    test_error_domain_register_mcp.cpp
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_domain_mcp.c
    ${CMAKE_SOURCE_DIR}/src/libs/error_domain/src/error_injection_mcp.c
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
        silibs::atu
        silibs::idsw_kng
        silibs::semaphore
        silibs::idhw
        silibs::idsw
        ms::cmocka::testrunner
        ms::service::atu_service
        ms::service::health_monitor
        ms::service::health_monitor::cli
        ms::lib::arm_intrinsic
        ms::lib::cortex_m7::nvic
        ms::lib::error_domain::mcp
        ms::lib::crash_dump
        ms::lib::scp_interrupts
        ms::cmsis::interface
)

# Generic ports implement within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ERROR_HANDLER "ms::lib::error_handler")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_THREADX "extern.threadx.win32")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")

target_include_directories(${TARGET}
    PRIVATE
        $<TARGET_PROPERTY:ms::lib::error_domain::mcp,INCLUDE_DIRECTORIES>
        ./
    SYSTEM PRIVATE
        # Include dirs for silibs_platform.c that we compile directly
        ${CMAKE_SOURCE_DIR}/.externs/repo/silibs/silibs_platform/include
        ${CMAKE_SOURCE_DIR}/.externs/repo/cortex_m7/src/externs/silibs_common/silibs_platform/inc
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MCP_RUNTIME_INIT
       FPFW_EVENT_TRACING_DISABLED
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
    fpfw_icc_base_send
    fpfw_icc_base_recv
    hm_submit_cper
    idsw_get_die_id
    mmio_read32
    mmio_write32
    nvic_global_enable
    nvic_global_disable
    nvic_irq_set_isr
    nvic_irq_enable
    nvic_irq_clear_pending
    nvic_irq_set_isr_with_param
    wdog_cmsdk_apb_disable
    wdog_cmsdk_apb_init
    idsw_get_platform_sdv
    is_cached_space
    mmu_fault
)
