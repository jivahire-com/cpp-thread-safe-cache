//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_crash_dump_init.c
 * Initialization of the crash dump.
 */

/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h>    // for DFWK_THREADX_HOST
#include <accelerator_ip.h>     // for accel_is_isolation_enabled
#include <addressblock0_regs.h> // for ADDRESSBLOCK0_WDOGLOAD_ADDRESS, ADDRESSBLOCK0_WDOGRIS_ADDRESS
#include <boot_status.h>        // for boot_status_req_t, boot_status_notify_extd
#include <bug_check.h>          // for BUG_CHECK
#include <crash_dump.h>         // for crash_dump_init
#include <crash_dump_dfwk.h>    // for crash_dump_device_t, crash_dump_interface_t
#include <crash_dump_memory.h>  // for CRASH_DUMP_MINI_HEADER_ADDR, CRASH_DUMP_MINI_HEADER_SIZE
#include <exception_handler.h>  // for exception_handler_init
#include <fpfw_init.h>          // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <fuses_csr_regs.h>
#include <fuses_top_regs.h>
#include <idhw.h>      // for idhw_get_cpu_type, idhw_get_die_id
#include <idsw.h>      // for idsw_get_cpu_type, idsw_get_die_id
#include <idsw_kng.h>  // for DIE_0, DIE_1
#include <kng_error.h> // for KNG_SUCCESS
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <scf_mhu_regs.h>
#include <scp_exp_csr_regs.h>
#include <scp_pwr_ctrl_regs.h>
#include <silibs_scp_exp_top_regs.h> // for SCP_EXP_TOP_SCF_RAM_ADDRESS, SCP_EXP_TOP_SCF_RAM_SIZE
#include <silibs_scp_top_regs.h> // for SCP_TOP_SCP_EXP_ADDRESS, SCP_TOP_SCP_INST_RAM_ADDRESS, SCP_TOP_SCP_INST_RAM_SIZE
#include <startup_shutdown.h> // for sos_register_ssi
#include <stddef.h>           // for NULL
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/
#define SCP_SCF_RAM_ADDRESS  (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_RAM_ADDRESS)
#define SCP_EXP_RAM0_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS)
#define SCP_EXP_RAM1_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
const core_register_mmio_t core_register_mmio[] = {
    // Crash dump register list :
    //     https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/SVT%20Documents/KNG/SDF/KNG_crashdump_reg_grading.xlsx?d=w03f0330eb98f45b5953fae51eef9f364&csf=1&web=1&e=wd00L1

    // SCP EXP FUSE
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_FUSE_ADDRESS + FUSES_TOP_FUSES_CSR_ADDRESS +
                      FUSES_CSR_SFCDMA_AGGREGATED_ERRSTATUS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_FUSE_ADDRESS + FUSES_TOP_FUSES_CSR_ADDRESS + FUSES_CSR_SFCRAM_ERRADDR_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_FUSE_ADDRESS + FUSES_TOP_FUSES_CSR_ADDRESS + FUSES_CSR_ERRSTATUS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},

    // SCP EXP SCF
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_SENSOR_RAM_ERROR_STATUS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_TEMP_STS_ERR_ADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_CUR_STS_ERR_ADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_VOLT_STS_ERR_ADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_DUAL_PORT_ERR_ADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},

    // SCP EXP CSR
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_SCP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_SCP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_HSP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_HSP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_REM_SCP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM0_REM_SCP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_SCP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_SCP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_HSP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_HSP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_REM_SCP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_RMSS_RAM1_REM_SCP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_SCP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_SCP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_HSP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_HSP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_REM_SCP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_SCFRAM_REM_SCP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_THERMAL_IO_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},

    // SCP Power Control
    {(volatile void*)(SCP_TOP_SCP_PWR_CTRL_ADDRESS + SCP_PWR_CTRL_SRAMECC_ERRSTATUS_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_PWR_CTRL_ADDRESS + SCP_PWR_CTRL_SRAMECC_ERRSTATUS_H_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_PWR_CTRL_ADDRESS + SCP_PWR_CTRL_SRAMECC_ERRADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_PWR_CTRL_ADDRESS + SCP_PWR_CTRL_SRAMECC_ERRADDR_H_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_PWR_CTRL_ADDRESS + SCP_PWR_CTRL_SRAMECC_ERRMISC1_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_PWR_CTRL_ADDRESS + SCP_PWR_CTRL_SRAMECC_ERRMISC1_H_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},

    // SCP RAS INIT CTRL
    {(volatile void*)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCODE_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_MSCP_ICERR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_MSCP_DCERR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},

    // Watchdog registers (WDOGRIS and WDOGMIS)
    {(volatile void*)(SCP_TOP_SCP_WATCHDOG_ADDRESS + ADDRESSBLOCK0_WDOGLOAD_ADDRESS + ADDRESSBLOCK0_WDOGRIS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(SCP_TOP_SCP_WATCHDOG_ADDRESS + ADDRESSBLOCK0_WDOGLOAD_ADDRESS + ADDRESSBLOCK0_WDOGMIS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL}};

/*------------- Functions ----------------*/
PLACED_CODE bool in_memory(uintptr_t start_addr, uintptr_t end_addr)
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

PLACED_CODE FPFW_INIT_COMPONENT(cd_init, FPFW_INIT_DEPENDENCIES("hw_ver", "gpio_lib", "boot_stat"))
{
    static crash_dump_type_context_t mini_dump_ctx = {.type = CRASH_DUMP_TYPE_MINI,
                                                      .mem_pool_addr = CRASH_DUMP_MINI_SCP_ADDR,
                                                      .mem_pool_size = CRASH_DUMP_MINI_SCP_SIZE,
                                                      .header = (crash_dump_header_t*)CRASH_DUMP_MINI_HEADER_ADDR};
    static crash_dump_context_t crash_dump_ctx = {
        .type_ctx = {NULL, NULL},
        .callbacks = {{.pre_dump_cb_count = 0, .post_dump_cb_count = 0}, {.pre_dump_cb_count = 0, .post_dump_cb_count = 0}},
        .core_index = CRASH_DUMP_CORE_SCP,
        .is_primary = false,
        .mmio_register_count = sizeof(core_register_mmio) / sizeof(core_register_mmio[0]),
        .mmio_registers = core_register_mmio,
        .in_memory = in_memory};

    // Get the DIE index
    crash_dump_ctx.die_index = idsw_get_die_id();

    // Set the semaphore key
    mini_dump_ctx.semaphore.id = SEM_ID_MSCP_EXP_0;
    mini_dump_ctx.semaphore.key = CRASH_DUMP_PROCESSOR_ID(crash_dump_ctx.die_index, crash_dump_ctx.core_index) + 1;

    // Initialize the crash dump
    crash_dump_init(&crash_dump_ctx);

    // Enable mini dump
    KNG_STATUS status = crash_dump_register_dump(&mini_dump_ctx);
    BUG_ASSERT_PARAM(status == KNG_SUCCESS, status, 0);

    // Initialize the exception handler
    status = exception_handler_init();
    BUG_ASSERT_PARAM(status == KNG_SUCCESS, status, 0);

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

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_CD_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_CD_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(cd_drv, FPFW_INIT_DEPENDENCIES("cd_init", "dfwk", "sos_int"))
{
    // Initialize the crash dump driver
    static crash_dump_device_t crash_dump_device;
    static crash_dump_interface_t crash_dump_interface;
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");

    crash_dump_device_initialize(&crash_dump_device, &(dfwk_host->Schedule));
    crash_dump_interface_initialize(&crash_dump_interface, &crash_dump_device);

    // static data for SSI registration
    static startup_ssi_registration_t ssi_registration = {
        .registration_id = SOS_SSI_ID_CRASHDUMP,
    };

    int32_t status =
        sos_register_ssi(fpfw_init_get_handle("sos_int"), &ssi_registration, &crash_dump_interface.Header);
    BUG_ASSERT_PARAM(status == FPFW_INIT_STATUS_SUCCESS, status, 0);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &crash_dump_interface};
}

PLACED_CODE FPFW_INIT_COMPONENT(cd_mhu_loc, FPFW_INIT_DEPENDENCIES("cd_init", "icc_mscp2mscp"))
{
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_LOCAL, fpfw_init_get_handle("icc_mscp2mscp"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(cd_mhu_rem, FPFW_INIT_DEPENDENCIES("cd_init", "icc_die2die", "hw_ver"))
{
    if (!idhw_is_single_die_boot_en())
    {
        // MHU transport D2D mailbox context.
        crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_MHU_REMOTE, fpfw_init_get_handle("icc_die2die"));
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(cd_spi_rem, FPFW_INIT_DEPENDENCIES("cd_init", "icc_d2dmbx", "hw_ver"))
{
    if (!idhw_is_single_die_boot_en())
    {
        // SPI transport D2D mailbox context.
        crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_SPI_REMOTE, fpfw_init_get_handle("icc_d2dmbx"));
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(cd_hsp, FPFW_INIT_DEPENDENCIES("cd_init", "icc_hspmbx"))
{
    crash_dump_config_icc(CRASH_DUMP_ICC_CONFIG_HSP, fpfw_init_get_handle("icc_hspmbx"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(cd_pomesh, FPFW_INIT_DEPENDENCIES("cd_init", "ddr", "hw_sem"))
{
    crash_dump_context_t* context = crash_dump_context();

    static crash_dump_type_context_t full_dump_ctx = {.type = CRASH_DUMP_TYPE_FULL,
                                                      .header = (crash_dump_header_t*)CRASH_DUMP_FULL_HEADER_ADDR};
    full_dump_ctx.mem_pool_addr = (uint64_t)(intptr_t)CRASH_DUMP_CORE_ADDRESS(context->die_index, context->core_index);
    full_dump_ctx.mem_pool_size = CRASH_DUMP_FULL_SIZE_PER_CORE;

    // Set the semaphore key
    // If initializing a crash dump on SVP use a local semaphore within the MSCP EXP Block.
    //   - See SVP Bug SVP bug https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/2327121
    // If initializing a full crash dump use a semaphore within the IOSS block.
    full_dump_ctx.semaphore.id = IS_PLATFORM_SVP() ? SEM_ID_MSCP_EXP_0 : SEM_ID_DIE0_IOSS_0;
    full_dump_ctx.semaphore.key = CRASH_DUMP_PROCESSOR_ID(context->die_index, context->core_index) + 1;

    KNG_STATUS status = crash_dump_register_dump(&full_dump_ctx);
    BUG_ASSERT_PARAM(status == KNG_SUCCESS, status, 0);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

PLACED_CODE FPFW_INIT_COMPONENT(cd_accel,
                                FPFW_INIT_DEPENDENCIES("cd_init", "cd_pomesh", "icc_sdm_mbx", "icc_cded_mbx", "hm_sdm", "hm_cded", "cfg_mgr"))
{
    for (ACCEL_ID accel_type = ACCEL_ID_SDM; accel_type < NUM_VALID_ACCEL_ID; accel_type++)
    {
        crash_dump_icc_config_t icc_config_type;
        fpfw_icc_base_ctx_t* icc_ctx;

        if (accel_is_isolation_enabled(accel_type))
        {
            // Skip if isolation is enabled for the given accelerator.
            continue;
        }

        if (accel_type == ACCEL_ID_SDM)
        {
            icc_config_type = CRASH_DUMP_ICC_CONFIG_SDM;
            icc_ctx = fpfw_init_get_handle("icc_sdm_mbx");
        }
        else
        {
            icc_config_type = CRASH_DUMP_ICC_CONFIG_CDED;
            icc_ctx = fpfw_init_get_handle("icc_cded_mbx");
        }

        crash_dump_config_icc(icc_config_type, icc_ctx);
        crash_dump_default_accel_cd_init(accel_type);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
