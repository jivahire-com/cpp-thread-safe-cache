//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mcp_crash_dump_init.c
 * Initialization of the crash dump.
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>              // for FPFW_RUNTIME_ASSERT
#include <addressblock0_regs.h>      // for ADDRESSBLOCK0_WDOGLOAD_ADDRESS, ADDRESSBLOCK0_WDOGRIS_ADDRESS
#include <bug_check.h>               // for BUG_CHECK
#include <crash_dump.h>              // for crash_dump_init
#include <crash_dump_memory.h>       // for CRASH_DUMP_MINI_HEADER_ADDR, CRASH_DUMP_MINI_HEADER_SIZE
#include <exception_handler.h>       // for exception_handler_init
#include <fpfw_init.h>               // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <idhw.h>                    // for idhw_get_cpu_type, idhw_get_die_id
#include <idsw.h>                    // for idsw_get_cpu_type, idsw_get_die_id
#include <idsw_kng.h>                // for DIE_0, DIE_1
#include <kng_error.h>               // for KNG_SUCCESS
#include <silibs_mcp_exp_top_regs.h> // for MCP_EXP_TOP_SCF_RAM_ADDRESS, MCP_EXP_TOP_SCF_RAM_SIZE
#include <silibs_mcp_top_regs.h>     // for MCP_TOP_MCP_EXP_ADDRESS
#include <stddef.h>                  // for NULL

/*-- Symbolic Constant Macros (defines) --*/
#define MCP_SCF_RAM_ADDRESS  (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_RAM_ADDRESS)
#define MCP_EXP_RAM0_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS)
#define MCP_EXP_RAM1_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS)

// This is the workaround until HSP handles warm reset after crash dump collection.
// Please remove this flag and the code guarded by it after HSP handles warm reset after crash dump collection.
#define WORKAROUND_HSP_WARM_RESET_REQ 1

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
const core_register_mmio_t core_register_mmio[] = {
    // https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484991
    // ToDo: Add MCP_EXP registers
    // ToDo: Add Power Control registers

    // Watchdog registers (WDOGRIS and WDOGMIS)
    {(volatile void*)(MCP_TOP_MCP_WATCHDOG_ADDRESS + ADDRESSBLOCK0_WDOGLOAD_ADDRESS + ADDRESSBLOCK0_WDOGRIS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_WATCHDOG_ADDRESS + ADDRESSBLOCK0_WDOGLOAD_ADDRESS + ADDRESSBLOCK0_WDOGMIS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL}};

/*------------- Functions ----------------*/
bool in_memory(uintptr_t start_addr, uintptr_t end_addr)
{
    // limit to RAM regions to avoid holes in register space
    if ((end_addr < MCP_TOP_MCP_INST_RAM_ADDRESS + MCP_TOP_MCP_INST_RAM_SIZE) ||
        (MCP_SCF_RAM_ADDRESS <= start_addr && end_addr < MCP_SCF_RAM_ADDRESS + MCP_EXP_TOP_SCF_RAM_SIZE) ||
        (MCP_TOP_MCP_DATA_RAM_ADDRESS <= start_addr && end_addr < MCP_TOP_MCP_DATA_RAM_ADDRESS + MCP_TOP_MCP_DATA_RAM_SIZE) ||
        (MCP_EXP_RAM0_ADDRESS <= start_addr && end_addr < MCP_EXP_RAM0_ADDRESS + MCP_EXP_TOP_RAM0_SIZE) ||
        (MCP_EXP_RAM1_ADDRESS <= start_addr && end_addr < MCP_EXP_RAM1_ADDRESS + MCP_EXP_TOP_RAM1_SIZE))
    {
        return true;
    }

    return false;
}

FPFW_INIT_COMPONENT(cd_init, FPFW_INIT_DEPENDENCIES("hw_ver", "gpio_lib", "hw_sem"))
{
    static crash_dump_type_context_t full_dump_ctx = {.type = CRASH_DUMP_TYPE_FULL,
                                                      .mem_pool_addr = CRASH_DUMP_FULL_MCP_ADDR,
                                                      .mem_pool_size = CRASH_DUMP_FULL_MCP_SIZE,
                                                      .header = (crash_dump_header_t*)CRASH_DUMP_FULL_HEADER_ADDR};

    static crash_dump_context_t crash_dump_ctx = {
        .type_ctx = {NULL, NULL},
        .callbacks = {{.pre_dump_cb_count = 0, .post_dump_cb_count = 0}, {.pre_dump_cb_count = 0, .post_dump_cb_count = 0}},
        .core_index = CRASH_DUMP_CORE_MCP,
        .is_primary = true,
        .mmio_register_count = sizeof(core_register_mmio) / sizeof(core_register_mmio[0]),
        .mmio_registers = core_register_mmio,
        .in_memory = in_memory};

    // Get the DIE index
    crash_dump_ctx.die_index = idsw_get_die_id();

    // Set the semaphore key
    // If initializing a crash dump on SVP use a local semaphore within the MSCP EXP Block.
    //   - See SVP Bug SVP bug https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/2327121
    // If initializing a full crash dump use a semaphore within the IOSS block.
    full_dump_ctx.semaphore.id = IS_PLATFORM_SVP() ? SEM_ID_MSCP_EXP_0 : SEM_ID_DIE0_IOSS_0;
    full_dump_ctx.semaphore.key = CRASH_DUMP_PROCESSOR_ID(crash_dump_ctx.die_index, crash_dump_ctx.core_index) + 1;

    // Initialize the crash dump
    crash_dump_init(&crash_dump_ctx);

    // Enable full dump
    KNG_STATUS status = crash_dump_register_dump(&full_dump_ctx);
    FPFW_RUNTIME_ASSERT(status == KNG_SUCCESS);

    // Initialize the exception handler
    status = exception_handler_init();
    FPFW_RUNTIME_ASSERT(status == KNG_SUCCESS);

    // Try to configure the crash dump ICC contexts if available.
    // If ICC contexts are not available yet, it will be configured later.
    // MHU transport MSCP local mailbox context.
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_LOCAL, fpfw_init_get_handle("icc_mscp2mscp"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cd_mhu_loc, FPFW_INIT_DEPENDENCIES("cd_init", "icc_mscp2mscp"))
{
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_LOCAL, fpfw_init_get_handle("icc_mscp2mscp"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

#ifdef WORKAROUND_HSP_WARM_RESET_REQ
FPFW_INIT_COMPONENT(cd_hsp, FPFW_INIT_DEPENDENCIES("cd_init", "icc_hspmbx"))
{
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_HSP, fpfw_init_get_handle("icc_hspmbx"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
#endif