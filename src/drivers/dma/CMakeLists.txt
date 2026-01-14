# Copyright (c) Microsoft Corporation. All rights reserved.

#
# DMA Driver - Src. files
#
set (SOURCES
    dma.c
    dma_interrupts.c
    dma_silibs_interface.c
    dma_timers.c
    dma_queue_management.c
    dma_events.c
)

#
# DMA Driver - SCP
#
set(TARGET kng.driver.dma.scp)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.dfwk
        fwlib.lib.interface.platform.timers
        fwlib.platform.dfwkhost.threadx
        ms::lib::error_codes
        ms::lib::threadx
        silibs::atu
        silibs::dmac
        silibs_platform
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.interrupts
        fwlib.lib.utils
        silibs::idsw_kng
        silibs::kng_silibs_platform
        ms::lib::arm_intrinsic
        ms::lib::cortex_m7::nvic
        ms::lib::crash_dump
        ms::lib::event_trace::trace
        ms::lib::scp_interrupts
        register_headers
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

add_library(ms::driver::dma::scp ALIAS ${TARGET})


#
# DMA Driver - MCP
#
set(TARGET kng.driver.dma.mcp)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${TARGET}
    PUBLIC
        fwlib.dfwk
        fwlib.lib.interface.platform.timers
        fwlib.platform.dfwkhost.threadx
        ms::lib::error_codes
        ms::lib::threadx
        silibs::atu
        silibs::dmac
        silibs_platform
    PRIVATE
        fwlib.lib.assert
        fwlib.lib.init
        fwlib.lib.interface.platform.errorhandler
        fwlib.lib.interface.platform.interrupts
        fwlib.lib.utils
        silibs::idsw_kng
        silibs::kng_silibs_platform
        ms::lib::arm_intrinsic
        ms::lib::cortex_m7::nvic
        ms::lib::crash_dump
        ms::lib::event_trace::trace
        ms::lib::mcp_interrupts
        register_headers
)

# Define interrupts during compile time for respective cores
target_compile_definitions(
    ${TARGET}
    PRIVATE
        DMA_FIRST_IRQ=HW_INT_MCP_DMAC_CH0_INT
        DMA_LAST_IRQ=HW_INT_MCP_DMAC_ATU_PARITY_INT
        DMA_BASE_ADDR=0x1480000UL # TODO: DMAC_BASE_ADDR to be defined in SiLibs  TODO:
        DMAC_CH0_INT=HW_INT_MCP_DMAC_CH0_INT
        DMAC_CH1_INT=HW_INT_MCP_DMAC_CH1_INT
        DMAC_REG_INT=HW_INT_MCP_DMAC_REG_INT
        DMAC_ATU_PARITY_INT=HW_INT_MCP_DMAC_ATU_PARITY_INT
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::driver::dma::mcp ALIAS ${TARGET})
