//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mcp_crash_dump_init.c
 * Initialization of the crash dump.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>         // for DfwkClientInterfaceOpen
#include <DfwkThreadXHost.h>    // for DFWK_THREADX_HOST
#include <FpFwAssert.h>         // for FPFW_RUNTIME_ASSERT
#include <addressblock0_regs.h> // for ADDRESSBLOCK0_WDOGLOAD_ADDRESS, ADDRESSBLOCK0_WDOGRIS_ADDRESS
#include <bug_check.h>          // for BUG_CHECK
#include <crash_dump.h>         // for crash_dump_init
#include <crash_dump_dfwk.h>    // for crash_dump_device_t, crash_dump_interface_t
#include <crash_dump_events.h>  // for CRASH_DUMP_ET
#include <crash_dump_memory.h>  // for CRASH_DUMP_MINI_HEADER_ADDR, CRASH_DUMP_MINI_HEADER_SIZE
#include <exception_handler.h>  // for exception_handler_init
#include <fpfw_init.h>          // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <fpfw_pldm_service.h>  // for pldm_platform_event_ready_notification
#include <fuses_csr_regs.h>
#include <fuses_top_regs.h>
#include <idhw.h>      // for idhw_get_cpu_type, idhw_get_die_id
#include <idsw.h>      // for idsw_get_cpu_type, idsw_get_die_id
#include <idsw_kng.h>  // for DIE_0, DIE_1
#include <kng_error.h> // for KNG_SUCCESS
#include <mcp_exp_csr_regs.h>
#include <mscp_ras_and_init_ctrl_registers_regs.h>
#include <scf_mhu_regs.h>
#include <silibs_mcp_exp_top_regs.h> // for MCP_EXP_TOP_SCF_RAM_ADDRESS, MCP_EXP_TOP_SCF_RAM_SIZE
#include <silibs_mcp_top_regs.h>     // for MCP_TOP_MCP_EXP_ADDRESS
#include <startup_shutdown.h>        // for sos_register_ssi
#include <stddef.h>                  // for NULL
#include <system_info.h>
/*-- Symbolic Constant Macros (defines) --*/
#define MCP_SCF_RAM_ADDRESS  (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_RAM_ADDRESS)
#define MCP_EXP_RAM0_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM0_ADDRESS)
#define MCP_EXP_RAM1_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_RAM1_ADDRESS)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
const core_register_mmio_t core_register_mmio[] = {
    // Crash dump register list :
    //     https://microsoft.sharepoint.com/:x:/r/teams/EchoFalls/SVT%20Documents/KNG/SDF/KNG_crashdump_reg_grading.xlsx?d=w03f0330eb98f45b5953fae51eef9f364&csf=1&web=1&e=wd00L1

    // MCP EXP FUSE
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_FUSE_ADDRESS + FUSES_TOP_FUSES_CSR_ADDRESS + FUSES_CSR_SFCRAM_ERRCTRL_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_FUSE_ADDRESS + FUSES_TOP_FUSES_CSR_ADDRESS + FUSES_CSR_SFCRAM_ERRADDR_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_FUSE_ADDRESS + FUSES_TOP_FUSES_CSR_ADDRESS + FUSES_CSR_ERRSTATUS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},

    // MCP EXP SCF
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_SENSOR_RAM_ERROR_STATUS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_TEMP_STS_ERR_ADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_CUR_STS_ERR_ADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_VOLT_STS_ERR_ADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_MHU_ADDRESS + SCF_MHU_DUAL_PORT_ERR_ADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},

    // MCP EXP CSR
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_MCP_EXP_CSR_ADDRESS + MCP_EXP_CSR_RMSS_RAM0_MCP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_MCP_EXP_CSR_ADDRESS + MCP_EXP_CSR_RMSS_RAM0_MCP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_MCP_EXP_CSR_ADDRESS + MCP_EXP_CSR_RMSS_RAM1_MCP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_MCP_EXP_CSR_ADDRESS + MCP_EXP_CSR_RMSS_RAM1_MCP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_MCP_EXP_CSR_ADDRESS + MCP_EXP_CSR_SCFRAM_MCP_ERRSTATUS_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_MCP_EXP_CSR_ADDRESS + MCP_EXP_CSR_SCFRAM_MCP_ERRADDR_REG_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_MCP_EXP_CSR_ADDRESS + MCP_EXP_CSR_MCP_IRPT_STATUS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},

    // MSCP RAS/INIT CTRL
    {(volatile void*)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRSTATUS_ADDRESS),
     1,
     FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCTRL_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRCODE_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_TCMECC_ERRADDR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_MSCP_ICERR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},
    {(volatile void*)(MCP_TOP_SCP_RAS_INIT_CTRL_ADDRESS + MSCP_RAS_AND_INIT_CTRL_REGISTERS_MSCP_DCERR_ADDRESS), 1, FPFW_CD_DUMP_PRIORITY_CRITICAL},

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

FPFW_INIT_COMPONENT(cd_drv, FPFW_INIT_DEPENDENCIES("cd_init", "dfwk", "sos_int"))
{
    // Initialize the crash dump driver
    static crash_dump_device_t crash_dump_device;
    static crash_dump_interface_t crash_dump_interface;
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");

    crash_dump_device_initialize(&crash_dump_device, &(dfwk_host->Schedule));
    crash_dump_interface_initialize(&crash_dump_interface, &crash_dump_device);

    // static data for SSI registration
    static startup_ssi_registration_t ssi_registration;
    int32_t status =
        sos_register_ssi(fpfw_init_get_handle("sos_int"), &ssi_registration, &crash_dump_interface.Header);
    FPFW_RUNTIME_ASSERT(status == FPFW_INIT_STATUS_SUCCESS);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &crash_dump_interface};
}

static void pldm_platform_event_ready_callback(uint16_t event_id, void* context)
{
    FPFW_UNUSED(event_id);
    FPFW_UNUSED(context);

    CRASH_DUMP_ET_INFO(CRASH_DUMP_ET_TYPE_PLDM_READY);
    crash_dump_transfer_full_dump_to_bmc();
}

FPFW_INIT_COMPONENT(cd_pldm, FPFW_INIT_DEPENDENCIES("cd_drv", "pldm"))
{
    if (idsw_get_die_id() == DIE_0)
    {
        pldm_platform_event_ready_notification ready_notification = {
            .CallBack = pldm_platform_event_ready_callback,
            .context = NULL, // No context needed for this callback
        };

        fpfw_status_t status = fpfw_pldm_service_register_platform_event_ready_notification(&ready_notification);
        FPFW_RUNTIME_ASSERT(status == FPFW_STATUS_SUCCESS);
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
