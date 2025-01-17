//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_init.c
 * Initialization of the crash dump.
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>        // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>         // for BUG_CHECK
#include <crash_dump.h>        // for crash_dump_init
#include <exception_handler.h> // for exception_handler_init
#include <fpfw_init.h>         // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_error.h> // for KNG_SUCCESS
#include <stddef.h>    // for NULL
#if defined(SCP_RUNTIME_INIT)
    #include <addressblock0_regs.h>      // for ADDRESSBLOCK0_WDOGLOAD_ADDRESS, ADDRESSBLOCK0_WDOGRIS_ADDRESS
    #include <silibs_scp_exp_top_regs.h> // for SCP_EXP_TOP_SCF_RAM_ADDRESS, SCP_EXP_TOP_SCF_RAM_SIZE
    #include <silibs_scp_top_regs.h> // for SCP_TOP_SCP_EXP_ADDRESS, SCP_TOP_SCP_INST_RAM_ADDRESS, SCP_TOP_SCP_INST_RAM_SIZE
#elif defined(MCP_RUNTIME_INIT)
    #include <addressblock0_regs.h>      // for ADDRESSBLOCK0_WDOGLOAD_ADDRESS, ADDRESSBLOCK0_WDOGRIS_ADDRESS
    #include <silibs_mcp_exp_top_regs.h> // for MCP_EXP_TOP_SCF_RAM_ADDRESS, MCP_EXP_TOP_SCF_RAM_SIZE
    #include <silibs_mcp_top_regs.h>     // for MCP_TOP_MCP_EXP_ADDRESS
#else
static_assert(false, "Unsupported CPU type");
#endif

/*-- Symbolic Constant Macros (defines) --*/
#if defined(SCP_RUNTIME_INIT)
    #define SCP_SCF_RAM_ADDRESS  (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
    #define SCP_EXP_RAM0_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
    #define SCP_EXP_RAM1_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)
#elif defined(MCP_RUNTIME_INIT)
    #define MCP_SCF_RAM_ADDRESS  (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_RAM_ADDRESS)
    #define MCP_EXP_RAM0_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS)
    #define MCP_EXP_RAM1_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS)
#else
static_assert(false, "Unsupported CPU type");
#endif

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#if defined(SCP_RUNTIME_INIT)
const core_register_mmio_t core_register_mmio[] = {
    // https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484991
    // ToDo: Add SCP_EXP registers
    // ToDo: Add Power Control registers

    // Watchdog registers (WDOGRIS and WDOGMIS)
    {(volatile void*)(SCP_TOP_SCP_WATCHDOG_ADDRESS + ADDRESSBLOCK0_WDOGLOAD_ADDRESS + ADDRESSBLOCK0_WDOGRIS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_WATCHDOG_ADDRESS + ADDRESSBLOCK0_WDOGLOAD_ADDRESS + ADDRESSBLOCK0_WDOGMIS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL}};
#elif defined(MCP_RUNTIME_INIT)
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
#else
static_assert(false, "Unsupported CPU type");
#endif

/*------------- Functions ----------------*/
bool in_memory(uintptr_t start_addr, uintptr_t end_addr)
{
    // limit to RAM regions to avoid holes in register space
#if defined(SCP_RUNTIME_INIT)
    if ((end_addr < SCP_TOP_SCP_INST_RAM_ADDRESS + SCP_TOP_SCP_INST_RAM_SIZE) ||
        (SCP_SCF_RAM_ADDRESS <= start_addr && end_addr < SCP_SCF_RAM_ADDRESS + SCP_EXP_TOP_SCF_RAM_SIZE) ||
        (SCP_TOP_SCP_DATA_RAM_ADDRESS <= start_addr && end_addr < SCP_TOP_SCP_DATA_RAM_ADDRESS + SCP_TOP_SCP_DATA_RAM_SIZE) ||
        (SCP_EXP_RAM0_ADDRESS <= start_addr && end_addr < SCP_EXP_RAM0_ADDRESS + SCP_EXP_TOP_RAM0_SIZE) ||
        (SCP_EXP_RAM1_ADDRESS <= start_addr && end_addr < SCP_EXP_RAM1_ADDRESS + SCP_EXP_TOP_RAM1_SIZE))
#elif defined(MCP_RUNTIME_INIT)
    if ((end_addr < MCP_TOP_MCP_INST_RAM_ADDRESS + MCP_TOP_MCP_INST_RAM_SIZE) ||
        (MCP_SCF_RAM_ADDRESS <= start_addr && end_addr < MCP_SCF_RAM_ADDRESS + MCP_EXP_TOP_SCF_RAM_SIZE) ||
        (MCP_TOP_MCP_DATA_RAM_ADDRESS <= start_addr && end_addr < MCP_TOP_MCP_DATA_RAM_ADDRESS + MCP_TOP_MCP_DATA_RAM_SIZE) ||
        (MCP_EXP_RAM0_ADDRESS <= start_addr && end_addr < MCP_EXP_RAM0_ADDRESS + MCP_EXP_TOP_RAM0_SIZE) ||
        (MCP_EXP_RAM1_ADDRESS <= start_addr && end_addr < MCP_EXP_RAM1_ADDRESS + MCP_EXP_TOP_RAM1_SIZE))
#else
    static_assert(false, "Unsupported CPU type");
#endif
    {
        return true;
    }

    return false;
}

FPFW_INIT_COMPONENT(cd_init, FPFW_INIT_DEPENDENCIES("hw_ver", "gpio_lib"))
{
    static crash_dump_config_t config = {.dump_type = FPFW_CD_DUMP_TYPE_NONE,
                                         .mmio_register_count =
                                             sizeof(core_register_mmio) / sizeof(core_register_mmio[0]),
                                         .mmio_registers = core_register_mmio,
                                         .in_memory = in_memory};

    // Get the DIE index
    config.die_index = idsw_get_die_id();

    // Set the core index and primary core flag
    switch (idsw_get_cpu_type())
    {
    case CPU_SCP:
        config.core_index = CRASH_DUMP_CORE_SCP;
        config.is_primary = false;
        break;
    case CPU_MCP:
        config.core_index = CRASH_DUMP_CORE_MCP;
        config.is_primary = true;
        break;
    default:
        FPFW_RUNTIME_ASSERT(false); // Unsupported CPU type
        break;
    }

    crash_dump_init(&config);

    int32_t status = exception_handler_init();
    FPFW_RUNTIME_ASSERT(status == KNG_SUCCESS);

    //    BUG_CHECK(4545, 0, 0);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cd_mhu_loc, FPFW_INIT_DEPENDENCIES("cd_init", "icc_mscp2mscp"))
{
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_LOCAL, fpfw_init_get_handle("icc_mscp2mscp"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

#ifdef SCP_RUNTIME_INIT
/* Todo: Initialize Crash dump ICC MHU remote once MHU transport ICC for D2D available
         https://azurecsi.visualstudio.com/Dev/_queries/query/7b903ea9-33d9-4830-898e-528d8fb03df7/
FPFW_INIT_COMPONENT(cd_mhu_rem, FPFW_INIT_DEPENDENCIES("cd_init", "icc_tbd", "hw_ver"))
{
    if (!idhw_is_single_die_boot_en())
    {
        // MHU transport D2D mailbox context.
        crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_REMOTE, fpfw_init_get_handle("icc_tbd"));
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
*/

FPFW_INIT_COMPONENT(cd_spi_rem, FPFW_INIT_DEPENDENCIES("cd_init", "icc_d2dmbx", "hw_ver"))
{
    if (!idhw_is_single_die_boot_en())
    {
        // SPI transport D2D mailbox context.
        crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_SPI_REMOTE, fpfw_init_get_handle("icc_d2dmbx"));
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cd_hsp, FPFW_INIT_DEPENDENCIES("cd_init", "icc_hspmbx"))
{
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_HSP, fpfw_init_get_handle("icc_hspmbx"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cd_pomesh, FPFW_INIT_DEPENDENCIES("cd_init", "ddr"))
{
    crash_dump_enable_full_dump(true);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
#endif