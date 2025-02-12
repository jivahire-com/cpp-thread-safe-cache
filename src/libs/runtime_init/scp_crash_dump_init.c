//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_crash_dump_init.c
 * Initialization of the crash dump.
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>              // for FPFW_RUNTIME_ASSERT
#include <addressblock0_regs.h>      // for ADDRESSBLOCK0_WDOGLOAD_ADDRESS, ADDRESSBLOCK0_WDOGRIS_ADDRESS
#include <bug_check.h>               // for BUG_CHECK
#include <crash_dump.h>              // for crash_dump_init
#include <exception_handler.h>       // for exception_handler_init
#include <fpfw_init.h>               // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <idhw.h>                    // for idhw_get_cpu_type, idhw_get_die_id
#include <idsw.h>                    // for idsw_get_cpu_type, idsw_get_die_id
#include <idsw_kng.h>                // for DIE_0, DIE_1
#include <kng_error.h>               // for KNG_SUCCESS
#include <silibs_scp_exp_top_regs.h> // for SCP_EXP_TOP_SCF_RAM_ADDRESS, SCP_EXP_TOP_SCF_RAM_SIZE
#include <silibs_scp_top_regs.h> // for SCP_TOP_SCP_EXP_ADDRESS, SCP_TOP_SCP_INST_RAM_ADDRESS, SCP_TOP_SCP_INST_RAM_SIZE
#include <stddef.h>              // for NULL

/*-- Symbolic Constant Macros (defines) --*/
#define SCP_SCF_RAM_ADDRESS  (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
#define SCP_EXP_RAM0_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define SCP_EXP_RAM1_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
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

/*------------- Functions ----------------*/
bool in_memory(uintptr_t start_addr, uintptr_t end_addr)
{
    // limit to RAM regions to avoid holes in register space
    if ((end_addr < SCP_TOP_SCP_INST_RAM_ADDRESS + SCP_TOP_SCP_INST_RAM_SIZE) ||
        (SCP_SCF_RAM_ADDRESS <= start_addr && end_addr < SCP_SCF_RAM_ADDRESS + SCP_EXP_TOP_SCF_RAM_SIZE) ||
        (SCP_TOP_SCP_DATA_RAM_ADDRESS <= start_addr && end_addr < SCP_TOP_SCP_DATA_RAM_ADDRESS + SCP_TOP_SCP_DATA_RAM_SIZE) ||
        (SCP_EXP_RAM0_ADDRESS <= start_addr && end_addr < SCP_EXP_RAM0_ADDRESS + SCP_EXP_TOP_RAM0_SIZE) ||
        (SCP_EXP_RAM1_ADDRESS <= start_addr && end_addr < SCP_EXP_RAM1_ADDRESS + SCP_EXP_TOP_RAM1_SIZE))
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
    if (idsw_get_cpu_type() == CPU_SCP)
    {
        config.core_index = CRASH_DUMP_CORE_SCP;
        config.is_primary = false;
    }
    else
    {
        FPFW_RUNTIME_ASSERT(false); // Unexpected CPU type
    }

    // Set the semaphore key
    config.cd_semaphore.semaphore_key = CRASH_DUMP_PROCESSOR_ID(config.die_index, config.core_index) + 1;

    // Initialize the crash dump
    crash_dump_init(&config);

    // Initialize the exception handler
    int32_t status = exception_handler_init();
    FPFW_RUNTIME_ASSERT(status == KNG_SUCCESS);

    // Try to configure the crash dump ICC contexts if available.
    // If ICC contexts are not available yet, it will be configured later.
    if (!idhw_is_single_die_boot_en())
    {
        // SPI transport D2D mailbox context.
        crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_SPI_REMOTE, fpfw_init_get_handle("icc_d2dmbx"));

        // MHU transport D2D mailbox context.
        crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_REMOTE, fpfw_init_get_handle("icc_die2die"));
    }
    // HSP mailbox context.
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_HSP, fpfw_init_get_handle("icc_hspmbx"));
    // MHU transport MSCP local mailbox context.
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_LOCAL, fpfw_init_get_handle("icc_mscp2mscp"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cd_mhu_loc, FPFW_INIT_DEPENDENCIES("cd_init", "icc_mscp2mscp"))
{
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_LOCAL, fpfw_init_get_handle("icc_mscp2mscp"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cd_mhu_rem, FPFW_INIT_DEPENDENCIES("cd_init", "icc_die2die", "hw_ver"))
{
    if (!idhw_is_single_die_boot_en())
    {
        // MHU transport D2D mailbox context.
        crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_REMOTE, fpfw_init_get_handle("icc_die2die"));
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

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

FPFW_INIT_COMPONENT(cd_pomesh, FPFW_INIT_DEPENDENCIES("cd_init", "ddr", "hw_sem"))
{
    crash_dump_enable_full_dump(true);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cd_accel, FPFW_INIT_DEPENDENCIES("cd_init", "cd_pomesh", "icc_sdm_mbx", "icc_cded_mbx"))
{
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_SDM, fpfw_init_get_handle("icc_sdm_mbx"));
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_CDED, fpfw_init_get_handle("icc_cded_mbx"));
    crash_dump_register_accel_ext_mmio();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
