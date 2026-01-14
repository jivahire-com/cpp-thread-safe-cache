# Copyright (c) Microsoft Corporation. All rights reserved.

#
# DMA unit test - Src. files
#
set (SOURCES
    test_dma_mscp.cpp
    dma_mocks.c
)

set (LIBRARIES
    PUBLIC
        fwlib.dfwk
        fwlib.lib.interface.platform.timers
        fwlib.platform.dfwkhost.threadx
        ms::lib::error_codes
        ms::lib::error_handler
        ms::lib::threadx
        silibs::atu
        silibs::dmac
        silibs_platform
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.interface.platform.interrupts
        fwlib.lib.utils
        silibs::idsw_kng
        silibs::kng_silibs_platform
        ms::cmocka::mocks
        ms::cmocka::testrunner
        ms::lib::cortex_m7::nvic
)

#
# DMA unit tests
#
set(TARGET dma.mscp.test)

# Compile test
add_executable(${TARGET} ${SOURCES})

# Generic ports implmented within this project
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_ATOMIC "ms::lib::atomics")
set_target_properties(${TARGET} PROPERTIES FPFW_PLATFORM_IMPL_INTERRUPTS "ms::lib::interrupts")

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET} ${LIBRARIES}
    PRIVATE
        ms::driver::dma::scp
        ms::lib::scp_interrupts
)

# Define interrupts during compile time for respective cores
target_compile_definitions(
    ${TARGET}
    PRIVATE
        DMA_FIRST_IRQ=HW_INT_SCP_DMAC_CH0_INT
        DMA_LAST_IRQ=HW_INT_SCP_DMAC_ATU_PARITY_INT
        DMA_BASE_ADDR=0x1580000UL
        # DMAC_BASE_ADDR to be defined in SiLibs  TODO:
        DMAC_CH0_INT=HW_INT_SCP_DMAC_CH0_INT
        DMAC_CH1_INT=HW_INT_SCP_DMAC_CH1_INT
        DMAC_REG_INT=HW_INT_SCP_DMAC_REG_INT
        DMAC_ATU_PARITY_INT=HW_INT_SCP_DMAC_ATU_PARITY_INT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})
unit_test_target(${TARGET})

#mock the functions needed
mock_target(${TARGET}
    DfwkDeviceInitialize
    DfwkAsyncRequestComplete
    DfwkQueueEnqueueRequest
    FpFwLockAcquire
    FpFwLockRelease
    atu_translate_address
    dmac_start_single_block_transfer
    idsw_get_cpu_type
    mmio_read64
    mmio_write64
    nvic_irq_set_isr_with_param
    nvic_irq_clear_pending
    nvic_irq_enable
    get_dmac_id_reg
    get_dmac_compver_reg
    set_dmac_reset_reg
    get_dmac_reset_reg
    get_dmac_cfg_reg
    set_dmac_cfg_reg
    get_dmac_chen_reg
    dmac_get_ch_interrupt_status
    dmac_disable_channel
    dmac_enable_channel
    dmac_abort_transfer

    # Threadx mocks
    _txe_timer_activate
    _txe_timer_create
    _txe_timer_delete
    _txe_timer_deactivate
)