//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip.c
 * This file provides interface to initializes the Accelerator IP(s) available
 * on the SoC.
 */

/*-------------------------------- Features ---------------------------------*/

/*-------------------------------- Includes ---------------------------------*/
#include "accelerator_ip.h"

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <_addressblock_0x100000_regs.h>
#include <accel_intr.h>          // for accel_intr_irq_init
#include <accelerator_ip_priv.h> // for get_accelerator_ctxt
#include <atu_lib.h>             // for atu_map, atu_unmap, atu_map...
#include <bug_check.h>           // for BUG_ASSERT
#include <cdedss_config_regs.h>  // for CDEDSS_CONFIG_SDM_EXT_CFG_ADDRESS
#include <fpfw_icc_base.h>       // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_icc_base_i.h>     // for fpfw_icc_base_ctx_t
#include <fpfw_init.h>
#include <fpfw_mbox_icc_transport.h> // for ICC_MBX_ASYNC_RECV, ICC_MBX_ASY...
#include <hsp_firmware_headers.h>
#include <idsw.h> // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>
#include <kng_soc_constants.h> // for DIE_INSTANCE
#include <sdm_ext_cfg_regs.h>  // for SDM_EXT_CFG_EMCPU_TCM_ITCM_ADDRESS, SDM_EXT_CFG_EMCPU_TCM_ITCM_SIZE
#include <sdmss_config_regs.h> // for SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS
#include <silibs_common.h>     // for ALIGN_UP
#include <silibs_platform.h>   // for debug_print, MMIO_UPDATE32
#include <silibs_status.h>     // for SILIBS_SUCCESS
#include <stdint.h>            // for int32_t, uintptr_t, uint32_t
#include <stdio.h>             // for printf, NULL
#include <string.h>            // for memcpy
#include <system_info.h>       // for is_hsp_present()
#include <tower_cdedss.h>      // for configure_cdedss_hsp_system_addr_map()

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

#define ACCEL_PCR_CONFIG_TIMEOUT (10)

#define SLEEP_100_MS                                (100)
#define MAX_RETRY_CNT_FOR_SMMU_CONFIGURE_GPBA_CHECK (50)

/* TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023639/ to revisit after boot chain stabilizes */
#define HSSS_DFP_TOP_KD_CDEDSS_HSP_AXI_ADDRESS (0xffffff0000000U)

#define ITCM_BIT_EN  0x1
#define ITCM_BIT_DIS 0x0

/*-------------------------------- Typedefs ---------------------------------*/

// TODO ADO: 1994002
typedef enum
{
    EMCPU_RECOVERY_START,
    EMCPU_RESET_SEQ_START,
    EMCPU_RESET_SEQ_COMPLETE,
    EMCPU_MBOX_RSP_RXED,
    EMCPU_ITCM_FW_LOAD_INVOKED,
    EMCPU_DTCM_FW_LOAD_INVOKED,
    EMCPU_CPU_RECOVERY_COMPLETE,
} emcpu_stage_t;

typedef enum
{
    EMCPU_ITCM_LOAD,
    EMCPU_DTCM_LOAD,
    EMCPU_RELEASE_CPU_WAIT,
} emcpu_fw_load_t;

typedef struct _emcpu_mbox_buffs
{
    kng_hsp_mailbox_msg recv_buff;                   // Buffer to hold HSP response for FW load
    kng_hsp_mailbox_cmd_load_fw_64bit_req send_buff; // Buffer to hold HSP FW load command
} emcpu_mbox_buffs_t;

typedef struct _emcpu_mbox_struct
{
    fpfw_icc_base_recv_req_t recv_params;
    fpfw_icc_base_send_req_t send_params;
    emcpu_fw_load_t load_stage;
    bool is_cold_boot;
} emcpu_mbox_struct_t;

typedef struct _emcpu_rst_struct
{
    emcpu_mbox_struct_t mbox_params;
    crash_dump_cb_t cb_fun;      // Callback post embed cpu recovery (one per accelerator instance)
    void* cb_ctx;                // Context to be pass to the function (one per accelerator instance)
    emcpu_stage_t cpu_rst_stage; // Variable to track current stage of recovery
} emcpu_rst_struct_t;

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/
/**
 * Store ATU mapped address
 */
static uint32_t accel_intr_atu_map_address[MAX_ACCELERATOR_TYPES];

// Forward declaring static function to allow for creation of struct
static void request_accel_fw_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status);
static void request_send_complete_cb(void* context, fpfw_status_t status);
static fpfw_status_t invoke_hsp_accel_fw_download(subsystem_ctxt_t* p_ss_ctxt);

/*------------------- Declarations (Statics and globals) --------------------*/

// TODO ADO: 1994002
static emcpu_mbox_buffs_t mbox_buffs[MAX_ACCELERATOR_TYPES] = {
    {.send_buff =
         {
             .header = {.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ, .context = 0},
         }},
    {.send_buff = {
         .header = {.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ, .context = 0},
     }}};

static emcpu_rst_struct_t cpu_rst_info[MAX_ACCELERATOR_TYPES] = {
    {.mbox_params = {.recv_params = {.payload_buffer = &mbox_buffs[ACCELERATOR_SDMSS].recv_buff,
                                     .buffer_size = sizeof(mbox_buffs[ACCELERATOR_SDMSS].recv_buff),
                                     .cb = request_accel_fw_load_complete_notify,
                                     .recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP},
                     .send_params = {.payload_buffer = &mbox_buffs[ACCELERATOR_SDMSS].send_buff,
                                     .buffer_size = sizeof(mbox_buffs[ACCELERATOR_SDMSS].send_buff),
                                     .cb = request_send_complete_cb}},
     .cpu_rst_stage = EMCPU_CPU_RECOVERY_COMPLETE},
    {.mbox_params = {.recv_params = {.payload_buffer = &mbox_buffs[ACCELERATOR_CDEDSS].recv_buff,
                                     .buffer_size = sizeof(mbox_buffs[ACCELERATOR_CDEDSS].recv_buff),
                                     .cb = request_accel_fw_load_complete_notify,
                                     .recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP},
                     .send_params = {.payload_buffer = &mbox_buffs[ACCELERATOR_CDEDSS].send_buff,
                                     .buffer_size = sizeof(mbox_buffs[ACCELERATOR_CDEDSS].send_buff),
                                     .cb = request_send_complete_cb}},
     .cpu_rst_stage = EMCPU_CPU_RECOVERY_COMPLETE}};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/
/*
TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023639/ to revisitthe following functions after boot chain stabilizes:
- init_hsp_cdedss_tower_atu_map
- init_hsp_cdedss_tower_atu_unmap
- init_hsp_cdedss_tower
*/
static int32_t init_hsp_cdedss_tower_atu_map(atu_map_entry_t* p_cdeedss_tower_atu_map_entry)
{
    p_cdeedss_tower_atu_map_entry->mscp_start_address = 0;                                    // 0x78000000;
    p_cdeedss_tower_atu_map_entry->mscp_end_address = ALIGN_UP(0x8000000, ATU_PAGE_SIZE) - 1; // 0x7fffffff;

    p_cdeedss_tower_atu_map_entry->attribute.axprot0 = 0x3;
    p_cdeedss_tower_atu_map_entry->attribute.axprot1 = 0x2;
    p_cdeedss_tower_atu_map_entry->attribute.axnse = 0x3;

    int32_t ret = atu_map(ATU_ID_MSCP, p_cdeedss_tower_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("accel_lib: WA: CDEDSS Tower ATU Mapping failed\n");
        return ACCEL_RET_FAIL_ATU_MAP;
    }

    return ACCEL_RET_SUCCESS;
}

static int32_t init_hsp_cdedss_tower_atu_unmap(atu_map_entry_t* p_cdeedss_tower_atu_map_entry)
{
    // TODO: WA until HSP configures CDEDSS Tower
    int32_t ret = atu_unmap(ATU_ID_MSCP, p_cdeedss_tower_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Accel IP CDEDSS Tower ATU Unmapping failed\n");
        return ACCEL_RET_FAIL_ATU_UNMAP;
    }

    return ACCEL_RET_SUCCESS;
}

static int32_t init_hsp_cdedss_tower(atu_map_entry_t* p_cdeedss_tower_atu_map_entry)
{
    // Create ATU Mapping (SCP View) for the CDEDSS Tower
    p_cdeedss_tower_atu_map_entry->ap_base_address = (uint64_t)HSSS_DFP_TOP_KD_CDEDSS_HSP_AXI_ADDRESS;

    // Create ATU Mapping (SCP View) for the Accel IP CDEDSS Tower
    int32_t ret = init_hsp_cdedss_tower_atu_map(p_cdeedss_tower_atu_map_entry);
    if (ret != ACCEL_RET_SUCCESS)
    {
        return ret;
    }

    uint32_t cdedss_tower_atu_mapped_addr = p_cdeedss_tower_atu_map_entry->mscp_start_address;
    CDEDSS_INSTANCE cdedss_id = 0;

    debug_print("accel_lib: WA: Initializing CDEDSS Tower\n");

    configure_cdedss_hsp_system_addr_map(HSSS_DFP_TOP_KD_CDEDSS_HSP_AXI_ADDRESS, cdedss_tower_atu_mapped_addr);
    configure_cdedss_vab_system_addr_map(cdedss_id, cdedss_tower_atu_mapped_addr);

    // Destroy ATU mapping
    ret = init_hsp_cdedss_tower_atu_unmap(p_cdeedss_tower_atu_map_entry);
    if (ret != ACCEL_RET_SUCCESS)
    {
        return ret;
    }

    debug_print("accel_lib: WA: CDEDSS Tower initialization complete.\n");

    return ret;
}

static eACCELERATOR_TYPE get_accelip_type(ACCELIP_SS_INSTANCE accel_type)
{
    switch (accel_type)
    {
    case D0_ACCELIP_SDMSS:
    case D1_ACCELIP_SDMSS: {
        return ACCELERATOR_SDMSS;
    }
    case D0_ACCELIP_CDEDSS:
    case D1_ACCELIP_CDEDSS: {
        return ACCELERATOR_CDEDSS;
    }
    default: {
        return MAX_ACCELERATOR_TYPES;
    }
    }
}

static void accel_fw_download(subsystem_ctxt_t* p_ss_ctxt, eACCELERATOR_TYPE accel_type)
{
    cpu_rst_info[accel_type].mbox_params.is_cold_boot = true;
    cpu_rst_info[accel_type].mbox_params.load_stage = EMCPU_ITCM_LOAD;

    /*
    If HSP is not present on the platform simply return. Do not request an HSP download
    TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023639/ to revisit after boot chain stabilizes
    */
    if (!system_info_is_hsp_present())
    {
        return;
    }

    /* CDED FW Download from HSP results in a Hard Fault. Bypassing invoke_hsp_accel_fw_download for SVP
    Raised ADO here: https://azurecsi.visualstudio.com/1P-SoC-Modeling/_workitems/edit/2026582 */
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        for (uint8_t dload_stage = 0; dload_stage <= EMCPU_RELEASE_CPU_WAIT; dload_stage++)
        {
            fpfw_status_t status = invoke_hsp_accel_fw_download(p_ss_ctxt);

            if (status != FPFW_STATUS_SUCCESS)
            {
                debug_print("Firmware download failed with status: %d\n", status);
                return;
            }

            // callback is explicitly invoked here because compiler incorrectly warns of a recursive call
            // if the callback is invoked inside the download function
            if (dload_stage != (EMCPU_RELEASE_CPU_WAIT))
            {
                // The last fw_download is to release cpuwait and cb should be skipped
                request_accel_fw_load_complete_notify(p_ss_ctxt, 0x0, status); // The output size is unused and hence passed as 0
            }
        }
    }
}

static int32_t init_accelerator(subsystem_ctxt_t* p_ss_ctxt)
{
    int32_t ret = ACCEL_RET_SUCCESS;
    eACCELERATOR_TYPE accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    if (accel_type == MAX_ACCELERATOR_TYPES)
    {
        debug_print("accel_lib: Invalid accel type\n");
        return ACCEL_RET_FAIL_INVALID_PARAMS;
    }

    if (accel_type == ACCELERATOR_CDEDSS)
    {
        /* TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023639/ to revisit after boot chain stabilizes */
        if ((idsw_get_platform_sdv() == PLATFORM_SVP_SIM) && (!system_info_is_hsp_present()))
        {
            atu_map_entry_t cdeedss_tower_atu_map_entry = {0};

            ret = init_hsp_cdedss_tower(&cdeedss_tower_atu_map_entry);
            if (ret != ACCEL_RET_SUCCESS)
            {
                debug_print("accel_lib: WA: CDEDSS Tower Init failed\n");
                return ret;
            }
        }
        else
        {
            /*
             * The HSP FW now supports CDEDSS Tower configurations.
             * The logic to trigger the configuration is being invoked from hsp_send_recv_post_scp_init_tower_config from tower.c
             */
        }
    }

    atu_map_entry_t atu_map_entry;
    memcpy((void*)&atu_map_entry, (void*)p_ss_ctxt->p_accelip_atu_map, sizeof(atu_map_entry_t));

    ret = atu_map(ATU_ID_MSCP, &atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        critical_print("Accel IP: init_accelerator: ATU MAP failed.\n");
        return ACCEL_RET_FAIL_ACCEL_IP;
    }
    debug_print("atu mapped for accel ip\n");

    accel_intr_atu_map_address[get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type)] = atu_map_entry.mscp_start_address;

    // Disable fw_preload_enabled if running on any Hardware, and if HSP is not present on that hardware
    if ((!system_info_is_hsp_present()) && (idsw_get_platform_sdv() != PLATFORM_SVP_SIM))
    {
        p_ss_ctxt->p_init_params->fw_preload_enabled = false;
        p_ss_ctxt->p_init_params->sdm_emcpu_init_cfg->release_m7_halt = true;
    }

    ret = accelip_ss_init(atu_map_entry.mscp_start_address, p_ss_ctxt->accelip_metadata.accel_type, p_ss_ctxt->p_init_params);
    if (ret != SILIBS_SUCCESS)
    {
        critical_print("Accel IP: init_accelerator: accelip ss init failed.\n");
        return ACCEL_RET_FAIL_SS_INIT;
    }

    /*
    TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023638/
    How do we handle SDM firmware download in SCP warm boot scenario?
     */
    info_print("%s: Invoking accel fw download\n", __func__);
    accel_fw_download(p_ss_ctxt, accel_type);
/**
 * TODO: Task 1982595: [SCP] Accel IP Move to Static ATU map
 */
#if 0
    ret = atu_unmap(ATU_ID_MSCP, &atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        critical_print("Accel IP: init_accelerator: ATU UNMAP failed.\n");
        return ACCEL_RET_FAIL_ACCEL_IP;
    }
    debug_print("atu unmapped for accel ip\n");
#endif

    /**
     * TODO: Task 1973445: [SCP] Move Accel Intr init in SCP after mailbox communication
     */
    printf("accel lib: Initialize accel interrupt\n");

    ret = accel_intr_irq_init(get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type));
    if (ret != ACCEL_INTR_RET_SUCCESS)
    {
        critical_print("Accel IP: init_accelerator: Accel Interrupt init failed.\n");
        return ACCEL_RET_FAIL_INTR_INIT;
    }

    return ACCEL_RET_SUCCESS;
}

// Need to replace with silibs APIs once implemented
// TODO: ADO: 1984231
static int _sdm_init_enable_cpuwait(const uintptr_t ext_cfg_addr)
{
    volatile ptr__addressblock_0x100000_reg reg_addr =
        (ptr__addressblock_0x100000_reg)(ext_cfg_addr + SDM_EXT_CFG_EMCPU_TCM_ITCM_SIZE + SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE);
    MMIO_UPDATE32(&reg_addr->emcpu_cfg_cpuwait,
                  _ADDRESSBLOCK_0X100000_EMCPU_CFG_CPUWAIT_EN_MASK,
                  _ADDRESSBLOCK_0X100000_EMCPU_CFG_CPUWAIT_EN_MASK);

    return SILIBS_SUCCESS;
}

// Need to replace with silibs APIs once implemented
// TODO: ADO: 1984231
static int _sdm_init_assert_nsysreset(const uintptr_t ext_cfg_addr)
{
    volatile ptr__addressblock_0x100000_reg reg_addr =
        (ptr__addressblock_0x100000_reg)(ext_cfg_addr + SDM_EXT_CFG_EMCPU_TCM_ITCM_SIZE + SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE);
    MMIO_UPDATE32(&reg_addr->emcpu_cfg_rst_ctrl, 0x0, _ADDRESSBLOCK_0X100000_EMCPU_CFG_RST_CTRL_NSYSRESET_MASK);

    return SILIBS_SUCCESS;
}

// API toggles ITCM enable -> 0 -> 1
static void _sdm_init_itcm_enable_trigger_seq(const uintptr_t ext_cfg_addr)
{
    volatile ptr__addressblock_0x100000_reg reg_addr =
        (ptr__addressblock_0x100000_reg)(ext_cfg_addr + SDM_EXT_CFG_EMCPU_TCM_ITCM_SIZE + SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE);
    MMIO_UPDATE32(&reg_addr->emcpu_cfg_tcm_ctrl, ITCM_BIT_DIS, _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MASK);
    do
    {
        // Wait for ITCM EN to become 0
    } while ((MMIO_READ32(&reg_addr->emcpu_cfg_tcm_ctrl) >> _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MSB) &
             _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MASK);
    MMIO_UPDATE32(&reg_addr->emcpu_cfg_tcm_ctrl, ITCM_BIT_EN, _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MASK);
    do
    {
        // Wait for ITCM EN to become 1
    } while (!((MMIO_READ32(&reg_addr->emcpu_cfg_tcm_ctrl) >> _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MSB) &
               _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MASK));
}

static uint32_t get_accel_name_and_offset_addr(eACCELERATOR_TYPE accel_type, char* accel_name)
{
    switch (accel_type)
    {
    case ACCELERATOR_SDMSS:
        memcpy(accel_name, "SDMSS", strlen("SDMSS") + 1);
        return SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS;
        break;
    case ACCELERATOR_CDEDSS:
        memcpy(accel_name, "CDEDSS", strlen("CDEDSS") + 1);
        return CDEDSS_CONFIG_SDM_EXT_CFG_ADDRESS;
        break;
    default:
        BUG_ASSERT(false);
        break;
    }
    return SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS; // To satisfy x86 compiler
}

static void request_send_complete_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

static fpfw_status_t invoke_hsp_accel_fw_download(subsystem_ctxt_t* p_ss_ctxt)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    if (icc_ctx == NULL)
    {
        return FPFW_STATUS_FAIL;
    }

    eACCELERATOR_TYPE accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    char accel_name[7];
    uint32_t ext_cfg_offset_addr = get_accel_name_and_offset_addr(accel_type, accel_name);
    size_t recv_msg_size_bytes = 0x0;

    debug_print("%s: %s Accel CPU Boot Stage: %d\n",
                __func__,
                accel_name,
                cpu_rst_info[accel_type].mbox_params.load_stage);

    /* Form the packet to request HSP to load accelerator firmware.
    Each accelerator (SDM/CDED-SDM) has 2 bins to be loaded; one each for ITCM and DTCM.
    After both bins are requested, there's one more stage where the SCP releases the accelerator's CPU wait.
    This function executes 3 times for each accelerator in case of cold boot */
    mbox_buffs[accel_type].send_buff.header.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ;
    switch (cpu_rst_info[accel_type].mbox_params.load_stage)
    {
    case EMCPU_ITCM_LOAD:
        cpu_rst_info[accel_type].mbox_params.load_stage = EMCPU_DTCM_LOAD;
        mbox_buffs[accel_type].send_buff.id =
            (accel_type == ACCELERATOR_SDMSS) ? HSP_FIRMWARE_ID_SDM_ITCM : HSP_FIRMWARE_ID_CDED_ITCM;
        mbox_buffs[accel_type].send_buff.load_addr_low = p_ss_ctxt->mem_info.itcm_load_addr_low;
        mbox_buffs[accel_type].send_buff.load_addr_high = p_ss_ctxt->mem_info.itcm_load_addr_high;

        cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_ITCM_FW_LOAD_INVOKED;
        break;
    case EMCPU_DTCM_LOAD:
        cpu_rst_info[accel_type].mbox_params.load_stage = EMCPU_RELEASE_CPU_WAIT;
        mbox_buffs[accel_type].send_buff.id =
            (accel_type == ACCELERATOR_SDMSS) ? HSP_FIRMWARE_ID_SDM_DTCM : HSP_FIRMWARE_ID_CDED_DTCM;
        mbox_buffs[accel_type].send_buff.load_addr_low = p_ss_ctxt->mem_info.dtcm_load_addr_low;
        mbox_buffs[accel_type].send_buff.load_addr_high = p_ss_ctxt->mem_info.dtcm_load_addr_high;

        cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_DTCM_FW_LOAD_INVOKED;
        break;
    case EMCPU_RELEASE_CPU_WAIT:
        // Load complete, this is the final load callback hence returning
        sdm_init_disable_cpu_wait((accel_intr_atu_map_address[accel_type] + ext_cfg_offset_addr));
        debug_print("%s: %s Accel CPU is now running\n", __func__, accel_name);
        cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_CPU_RECOVERY_COMPLETE;
        // skip callback for cold boot
        if (!cpu_rst_info[accel_type].mbox_params.is_cold_boot)
        {
            cpu_rst_info[accel_type].cb_fun(cpu_rst_info[accel_type].cb_ctx);
        }
        return status;
    default:
        BUG_ASSERT(false);
        break;
    }

    if (cpu_rst_info[accel_type].mbox_params.is_cold_boot)
    {
        status = fpfw_icc_base_send_recv_sync(icc_ctx, &mbox_buffs[accel_type].send_buff, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);
    }
    else
    {
        status = fpfw_icc_base_recv(icc_ctx, &cpu_rst_info[accel_type].mbox_params.recv_params);
        BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);
        status = fpfw_icc_base_send(icc_ctx, &cpu_rst_info[accel_type].mbox_params.send_params);
        BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);
    }
    return status;
}

static void request_accel_fw_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    subsystem_ctxt_t* p_ss_ctxt = (subsystem_ctxt_t*)context;
    eACCELERATOR_TYPE accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_MBOX_RSP_RXED;
    debug_print("%s: Accel Load Complete RXed(0x%x)\n", __func__, status);
    BUG_ASSERT(status == FPFW_STATUS_SUCCESS);

    if (!cpu_rst_info[accel_type].mbox_params.is_cold_boot)
    {
        BUG_ASSERT(mbox_buffs[accel_type].recv_buff.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
        BUG_ASSERT(mbox_buffs[accel_type].recv_buff.rsp.status == FPFW_STATUS_SUCCESS);
        invoke_hsp_accel_fw_download(p_ss_ctxt);
    }
    else
    {
        BUG_ASSERT(mbox_buffs[accel_type].send_buff.header.cmd == HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP);
    }
}

static void emcpu_recovery_sequence(uintptr_t sdm_ext_cfg_base, subsystem_ctxt_t* p_ss_ctxt)
{
    /**
     * Sequence to reset Embed CPU without letting it execute
     * CPUWAIT      = 1
     * FENCE_EN     = 1
     * NSYSRESET    = 0
     * ITCM_EN      = 0
     * DELAY
     * ITCM_EN      = 1
     * FENCE_EN     = 0
     * NSYSRESET    = 1
     */
    eACCELERATOR_TYPE accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_RESET_SEQ_START;
    // TODO: ADO: 1982366 Replace implicit SDM INIT API implementations with proper SILIBS API once they're merged
    _sdm_init_enable_cpuwait(sdm_ext_cfg_base);
    sdm_init_enable_fence(sdm_ext_cfg_base, true);
    _sdm_init_assert_nsysreset(sdm_ext_cfg_base);
    // Currently only directly disabling/enabling ITCM EN
    _sdm_init_itcm_enable_trigger_seq(sdm_ext_cfg_base);
    sdm_init_enable_fence(sdm_ext_cfg_base, false);
    sdm_init_deassert_nsysreset(sdm_ext_cfg_base);
    cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_RESET_SEQ_COMPLETE;

    debug_print("%s: M7 in CPUWAIT\n", __func__);

    // TODO: ADO: 1984962 Temporary WAR to keep SDM in stable loop until HSP firmware download is enabled
    // This needs to be executed only when HSP is not present when FW download is implemented
    // Reset programming moved here since cpuwait will directly be released for non hsp platforms
    volatile uint32_t* fw_dest_addr = (uint32_t*)(sdm_ext_cfg_base + SDM_EXT_CFG_EMCPU_TCM_ITCM_ADDRESS);
    volatile uint32_t* fw_src_addr = (uint32_t*)(p_ss_ctxt->p_init_params->sdm_emcpu_fw_image_start_addr);
    size_t fw_image_size = (size_t)(p_ss_ctxt->p_init_params->sdm_emcpu_fw_image_size);
    size_t i;
    for (i = 0; i < fw_image_size; i += 4)
    {
        *fw_dest_addr++ = *fw_src_addr++;
    }

    /*
    Invoke FW download command only if HSP is available
    TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023639/ to revisit after boot chain stabilizes
    */
    if (system_info_is_hsp_present())
    {
        cpu_rst_info[accel_type].mbox_params.load_stage = EMCPU_ITCM_LOAD;
        cpu_rst_info[accel_type].mbox_params.is_cold_boot = false;
        invoke_hsp_accel_fw_download(p_ss_ctxt);
    }
    else
    {
        // Release SDM cpuwait and update stages and invoke callback
        cpu_rst_info[accel_type].mbox_params.load_stage = EMCPU_RELEASE_CPU_WAIT;
        sdm_init_disable_cpu_wait((sdm_ext_cfg_base));
        debug_print("%s: Accel CPU is now running\n", __func__);
        cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_CPU_RECOVERY_COMPLETE;
        cpu_rst_info[accel_type].cb_fun(cpu_rst_info[accel_type].cb_ctx);
    }

    debug_print("%s: FW LOAD SENT\n", __func__);
}

/*----------------------------- Global Functions ----------------------------*/

uint32_t accelerator_ip_get_atu_mapped_cfg_address(eACCELERATOR_TYPE accel_type)
{
    return accel_intr_atu_map_address[accel_type];
}

int32_t scp_accelerators_init(void)
{
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    int ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    FPFW_RUNTIME_ASSERT(p_ss_ctxt != NULL);

    printf("Number of Accelerator instances present: %d\n", (int)accel_ctxt_size);

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO (ADO 1728772) : init any particular accelerator instance only if that is enabled in fuse
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            printf("accel lib: Initializing for die_id = %d, accel_type = %d, accel_instance = %d\n",
                   p_ss_ctxt[index].accelip_metadata.die_instance,
                   p_ss_ctxt[index].accelip_metadata.accel_type,
                   p_ss_ctxt[index].accelip_metadata.accel_instance);

            ret = init_accelerator(&p_ss_ctxt[index]);

            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);
        }
    }

    return ret;
}

int32_t scp_accelerators_isolation_control(void)
{
    // TODO: read the knob (and fuse if implemented).  For now we are hardcoding to de-isolate
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    int32_t ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    FPFW_RUNTIME_ASSERT(p_ss_ctxt != NULL);

    printf("Number of Accelerator instances present: %d\n", (int)accel_ctxt_size);

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO (ADO 1728772) : init any particular accelerator instance only if that is enabled in fuse
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            debug_print("accel lib: Initializing for die_id = %d, accel_type = %d, accel_instance = %d\n",
                        p_ss_ctxt[index].accelip_metadata.die_instance,
                        p_ss_ctxt[index].accelip_metadata.accel_type,
                        p_ss_ctxt[index].accelip_metadata.accel_instance);

            atu_map_entry_t atu_map_entry;
            memcpy((void*)&atu_map_entry, (void*)p_ss_ctxt[index].p_accelip_atu_map, sizeof(atu_map_entry_t));

            ret = atu_map(ATU_ID_MSCP, &atu_map_entry);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);

            ret = accelip_ss_enable_ip_isolation(atu_map_entry.mscp_start_address,
                                                 p_ss_ctxt[index].accelip_metadata.accel_type,
                                                 p_ss_ctxt[index].p_init_params->accelip_ss_cfg->isolation_enable);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);

            ret = atu_unmap(ATU_ID_MSCP, &atu_map_entry);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);
        }
    }
    return ret;
}
/* TODO: Review use of BUG_ASSERT (vis a vis ASSERT) in accelIP init sequence https://azurecsi.visualstudio.com/Dev/_workitems/edit/2025877/ */
void scp_accelerators_emcpu_reset(eACCELERATOR_TYPE accel_type, crash_dump_cb_t cb_fun, void* cb_ctx)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    char accel_name[7];
    uint32_t ext_cfg_offset_addr = get_accel_name_and_offset_addr(accel_type, accel_name);

    BUG_ASSERT(accel_type < MAX_ACCELERATOR_TYPES);
    BUG_ASSERT(cb_fun != NULL);

    if (cpu_rst_info[accel_type].cpu_rst_stage != EMCPU_CPU_RECOVERY_COMPLETE)
    {
        critical_print("%s: %s Accel CPU already in recovery\n", __func__, accel_name);
        BUG_ASSERT(false);
    }

    cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_RECOVERY_START;
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO ADO: 1994002
        if ((p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance) &&
            (get_accelip_type(p_ss_ctxt[index].accelip_metadata.accel_type) == accel_type))
        {
            p_ss_ctxt = &p_ss_ctxt[index];
            break;
        }
    }

    cpu_rst_info[accel_type].cb_fun = cb_fun;
    cpu_rst_info[accel_type].cb_ctx = cb_ctx;
    cpu_rst_info[accel_type].mbox_params.recv_params.cb_ctx = (void*)p_ss_ctxt;

    debug_print("%s: %s Accel Invoking cpu recovery\n", __func__, accel_name);
    emcpu_recovery_sequence((ext_cfg_offset_addr + accel_intr_atu_map_address[accel_type]), p_ss_ctxt);
}