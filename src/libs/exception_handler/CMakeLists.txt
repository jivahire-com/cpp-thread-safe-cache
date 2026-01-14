# Copyright (c) Microsoft Corporation. All rights reserved.

# Exception handler for SCP
set(TARGET kng.lib.exception_handler.scp)

set (SOURCES
    src/exception_handler.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::lib::cortex_m7::nvic
        ms::lib::crash_dump
        ms::lib::error_codes
        ms::lib::error_domain::scp
        ms::lib::scp_interrupts
        ms::lib::threadx
        ms::service::health_monitor
        silibs::atu
        silibs::cmsdk_wd
        silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       SCP_RUNTIME_INIT
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_CE_MASK=SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_CE_MASK
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_UE_MASK=SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_UE_MASK
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_OF_MASK=SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_OF_MASK
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_ERRPOSITION_MASK=SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_ERRPOSITION_MASK
       MSCP_TOP_MSCP_EXP_ADDRESS=SCP_TOP_SCP_EXP_ADDRESS
       MSCP_EXP_TOP_SCF_RAM_ADDRESS=SCP_EXP_TOP_SCF_RAM_ADDRESS
       MSCP_EXP_TOP_SCF_RAM_SIZE=SCP_EXP_TOP_SCF_RAM_SIZE
       MSCP_EXP_TOP_MSCP_EXP_CSR_ADDRESS=SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_ADDRESS=SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_ADDRESS
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRADDR_REG_ADDRESS=SCP_EXP_CSR_SCFRAM_SCP_ERRADDR_REG_ADDRESS
       ACPI_ERROR_DOMAIN_MSCP_PROC=ACPI_ERROR_DOMAIN_SCP_PROC
       RECORD_ID_MSCP_SCF_RAM=RECORD_ID_SCP_SCF_RAM
       RECORD_ID_MSCP_WD=RECORD_ID_SCP_WD
       RECORD_ID_MSCP_EXCEPTION=RECORD_ID_SCP_EXCEPTION
       RECORD_ID_MSCP_ARSM_RAM=RECORD_ID_SCP_ARSM_RAM
       RECORD_ID_MSCP_RSM_RAM=RECORD_ID_SCP_RSM_RAM
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::exception_handler::scp ALIAS ${TARGET})

# Exception handler for SCP
set(TARGET kng.lib.exception_handler.mcp)

set (SOURCES
    src/exception_handler.c
)

add_library(${TARGET}
    STATIC
        ${SOURCES}
)

target_link_libraries(${TARGET}
    PRIVATE
        ms::lib::cortex_m7::nvic
        ms::lib::crash_dump
        ms::lib::error_codes
        ms::lib::error_domain::mcp
        ms::lib::mcp_interrupts
        ms::lib::threadx
        ms::service::health_monitor
        silibs::atu
        silibs::cmsdk_wd
        silibs::idsw_kng
)

target_include_directories(${TARGET}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_compile_definitions(
    ${TARGET}
    PRIVATE
       MCP_RUNTIME_INIT
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_CE_MASK=MCP_EXP_CSR_SCFRAM_MCP_ERRSTATUS_REG_CE_MASK
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_UE_MASK=MCP_EXP_CSR_SCFRAM_MCP_ERRSTATUS_REG_UE_MASK
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_OF_MASK=MCP_EXP_CSR_SCFRAM_MCP_ERRSTATUS_REG_OF_MASK
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_ERRPOSITION_MASK=MCP_EXP_CSR_SCFRAM_MCP_ERRSTATUS_REG_ERRPOSITION_MASK
       MSCP_TOP_MSCP_EXP_ADDRESS=MCP_TOP_MCP_EXP_ADDRESS
       MSCP_EXP_TOP_SCF_RAM_ADDRESS=MCP_EXP_TOP_SCF_RAM_ADDRESS
       MSCP_EXP_TOP_SCF_RAM_SIZE=MCP_EXP_TOP_SCF_RAM_SIZE
       MSCP_EXP_TOP_MSCP_EXP_CSR_ADDRESS=MCP_EXP_TOP_MCP_EXP_CSR_ADDRESS
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRSTATUS_REG_ADDRESS=MCP_EXP_CSR_SCFRAM_MCP_ERRSTATUS_REG_ADDRESS
       MSCP_EXP_CSR_SCFRAM_MSCP_ERRADDR_REG_ADDRESS=MCP_EXP_CSR_SCFRAM_MCP_ERRADDR_REG_ADDRESS
       ACPI_ERROR_DOMAIN_MSCP_PROC=ACPI_ERROR_DOMAIN_MCP_PROC
       RECORD_ID_MSCP_SCF_RAM=RECORD_ID_MCP_SCF_RAM
       RECORD_ID_MSCP_WD=RECORD_ID_MCP_WD
       RECORD_ID_MSCP_EXCEPTION=RECORD_ID_MCP_EXCEPTION
       RECORD_ID_MSCP_ARSM_RAM=RECORD_ID_MCP_ARSM_RAM
       RECORD_ID_MSCP_RSM_RAM=RECORD_ID_MCP_RSM_RAM
)

lint_target(${TARGET})
format_target(${TARGET})
coverage_target(${TARGET})

add_library(ms::lib::exception_handler::mcp ALIAS ${TARGET})