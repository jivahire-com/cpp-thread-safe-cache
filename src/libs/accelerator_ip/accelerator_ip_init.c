//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_init.c
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
#include <accelip_id.h>          // NUM_VALID_ACCEL_ID, ACCEL_ID_SDM, ACCEL_ID_CDED
#include <atu_init.h>            // for atu_svc_accel_atu_addr
#include <atu_lib.h>             // for atu_map, atu_unmap, atu_map...
#include <bug_check.h>           // for BUG_ASSERT
#include <cdedss_config_regs.h>  // for CDEDSS_CONFIG_SDM_EXT_CFG_ADDRESS
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h>   // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_icc_base_i.h> // for fpfw_icc_base_ctx_t
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
#include <string.h>            // for memcpy, strlen
#include <system_info.h>       // for system_info_is_hsp_present

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

#define ACCEL_NAME_LEN 8

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
uint32_t accel_intr_atu_map_address[NUM_VALID_ACCEL_ID] = {0};

// Forward declaring static function to allow for creation of struct
static void request_accel_fw_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status);
static void request_send_complete_cb(void* context, fpfw_status_t status);

/*------------------- Declarations (Statics and globals) --------------------*/

// TODO ADO: 1994002
static emcpu_mbox_buffs_t mbox_buffs[NUM_VALID_ACCEL_ID] = {
    {.send_buff =
         {
             .header = {.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ, .context = 0},
         }},
    {.send_buff = {
         .header = {.cmd = HSP_MAILBOX_CMD_LOAD_FW_64BIT_REQ, .context = 0},
     }}};

static emcpu_rst_struct_t cpu_rst_info[NUM_VALID_ACCEL_ID] = {
    {.mbox_params = {.recv_params = {.payload_buffer = &mbox_buffs[ACCEL_ID_SDM].recv_buff,
                                     .buffer_size = sizeof(mbox_buffs[ACCEL_ID_SDM].recv_buff),
                                     .cb = request_accel_fw_load_complete_notify,
                                     .recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP},
                     .send_params = {.payload_buffer = &mbox_buffs[ACCEL_ID_SDM].send_buff,
                                     .buffer_size = sizeof(mbox_buffs[ACCEL_ID_SDM].send_buff),
                                     .cb = request_send_complete_cb}},
     .cpu_rst_stage = EMCPU_CPU_RECOVERY_COMPLETE},
    {.mbox_params = {.recv_params = {.payload_buffer = &mbox_buffs[ACCEL_ID_CDED].recv_buff,
                                     .buffer_size = sizeof(mbox_buffs[ACCEL_ID_CDED].recv_buff),
                                     .cb = request_accel_fw_load_complete_notify,
                                     .recv_cmd_code = HSP_MAILBOX_CMD_LOAD_FW_64BIT_RSP},
                     .send_params = {.payload_buffer = &mbox_buffs[ACCEL_ID_CDED].send_buff,
                                     .buffer_size = sizeof(mbox_buffs[ACCEL_ID_CDED].send_buff),
                                     .cb = request_send_complete_cb}},
     .cpu_rst_stage = EMCPU_CPU_RECOVERY_COMPLETE}};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

static uint32_t get_accel_name_and_offset_addr(ACCEL_ID accel_type, char* accel_name)
{
    switch (accel_type)
    {
    case ACCEL_ID_SDM:
        memcpy(accel_name, "SDMSS", strlen("SDMSS") + 1);
        return SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS;
    case ACCEL_ID_CDED:
        memcpy(accel_name, "CDEDSS", strlen("CDEDSS") + 1);
        return CDEDSS_CONFIG_SDM_EXT_CFG_ADDRESS;
    default:
        BUG_ASSERT(false);
        break;
    }

    return SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS; // To satisfy x86 compiler
}

static fpfw_status_t invoke_hsp_accel_fw_download(subsystem_ctxt_t* p_ss_ctxt)
{
    fpfw_status_t status = FPFW_STATUS_SUCCESS;

    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    if (icc_ctx == NULL)
    {
        return FPFW_STATUS_FAIL;
    }

    ACCEL_ID accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    char accel_name[ACCEL_NAME_LEN];
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
            (accel_type == ACCEL_ID_SDM) ? HSP_FIRMWARE_ID_SDM_ITCM : HSP_FIRMWARE_ID_CDED_ITCM;
        mbox_buffs[accel_type].send_buff.load_addr_low = p_ss_ctxt->mem_info.itcm_load_addr_low;
        mbox_buffs[accel_type].send_buff.load_addr_high = p_ss_ctxt->mem_info.itcm_load_addr_high;
        cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_ITCM_FW_LOAD_INVOKED;
        break;

    case EMCPU_DTCM_LOAD:
        cpu_rst_info[accel_type].mbox_params.load_stage = EMCPU_RELEASE_CPU_WAIT;
        mbox_buffs[accel_type].send_buff.id =
            (accel_type == ACCEL_ID_SDM) ? HSP_FIRMWARE_ID_SDM_DTCM : HSP_FIRMWARE_ID_CDED_DTCM;
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

    if (system_info_is_hsp_present())
    {
        if (cpu_rst_info[accel_type].mbox_params.is_cold_boot)
        {
            status = fpfw_icc_base_send_recv_sync(icc_ctx,
                                                  &mbox_buffs[accel_type].send_buff,
                                                  sizeof(kng_hsp_mailbox_msg),
                                                  &recv_msg_size_bytes);
        }
        else
        {
            status = fpfw_icc_base_recv(icc_ctx, &cpu_rst_info[accel_type].mbox_params.recv_params);
            BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);
            status = fpfw_icc_base_send(icc_ctx, &cpu_rst_info[accel_type].mbox_params.send_params);
            BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);
        }
    }

    return status;
}

// API toggles ITCM enable -> 0 -> 1
static void sdm_init_itcm_toggle(const uintptr_t ext_cfg_addr)
{
    volatile ptr__addressblock_0x100000_reg reg_addr =
        (ptr__addressblock_0x100000_reg)(ext_cfg_addr + SDM_EXT_CFG_EMCPU_TCM_ITCM_SIZE + SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE);

    sdm_init_itcm_enable(ext_cfg_addr, false);
    do
    {
        // Wait for ITCM EN to become 0
    } while ((MMIO_READ32(&reg_addr->emcpu_cfg_tcm_ctrl) >> _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MSB) &
             _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MASK);

    sdm_init_itcm_enable(ext_cfg_addr, true);
    do
    {
        // Wait for ITCM EN to become 1
    } while (!((MMIO_READ32(&reg_addr->emcpu_cfg_tcm_ctrl) >> _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MSB) &
               _ADDRESSBLOCK_0X100000_EMCPU_CFG_TCM_CTRL_ITCM_EN_MASK));
}

static void request_send_complete_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

static void request_accel_fw_load_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    subsystem_ctxt_t* p_ss_ctxt = (subsystem_ctxt_t*)context;
    ACCEL_ID accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

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
    ACCEL_ID accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_RESET_SEQ_START;

    sdm_init_enable_cpuwait(sdm_ext_cfg_base);
    sdm_init_enable_fence(sdm_ext_cfg_base, true);
    sdm_init_assert_nsysreset(sdm_ext_cfg_base);

    // Currently only directly disabling/enabling ITCM EN
    sdm_init_itcm_toggle(sdm_ext_cfg_base);

    sdm_init_enable_fence(sdm_ext_cfg_base, false);
    sdm_init_deassert_nsysreset(sdm_ext_cfg_base);
    cpu_rst_info[accel_type].cpu_rst_stage = EMCPU_RESET_SEQ_COMPLETE;

    debug_print("%s: M7 in CPUWAIT\n", __func__);

    cpu_rst_info[accel_type].mbox_params.load_stage = EMCPU_ITCM_LOAD;
    cpu_rst_info[accel_type].mbox_params.is_cold_boot = false;
    invoke_hsp_accel_fw_download(p_ss_ctxt);

    debug_print("%s: FW LOAD SENT\n", __func__);
}

static int32_t init_accelerator(subsystem_ctxt_t* p_ss_ctxt)
{
    int32_t ret = ACCEL_RET_SUCCESS;
    ACCEL_ID accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    if (accel_type == NUM_VALID_ACCEL_ID)
    {
        debug_print("accel_lib: Invalid accel type\n");
        return ACCEL_RET_FAIL_INVALID_PARAMS;
    }

    accel_intr_atu_map_address[accel_type] = atu_svc_accel_atu_addr(accel_type);

    if (!system_info_is_hsp_present())
    {
        printf("accel lib: FPGA without HSP: Loading Reset Loop\n");
        p_ss_ctxt->p_init_params->fw_preload_enabled = false;
    }

    ret = accelip_ss_init(accel_intr_atu_map_address[accel_type],
                          p_ss_ctxt->accelip_metadata.accel_type,
                          p_ss_ctxt->p_init_params);
    if (ret != SILIBS_SUCCESS)
    {
        critical_print("Accel IP: init_accelerator: accelip_ss_init failed.\n");
        return ACCEL_RET_FAIL_SS_INIT;
    }

    /*
     * TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023638/
     * How do we handle SDM firmware download in SCP warm boot scenario?
     */

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
    if (!(IS_PLATFORM_SVP()))
    {
        /**
         * TODO: Task 1973445: [SCP] Move Accel Intr init in SCP after mailbox communication
         */
        printf("accel lib: Initialize accel interrupt\n");

        ret = accel_scp_intr_init(accel_type);
        if (ret != ACCEL_INTR_RET_SUCCESS)
        {
            critical_print("Accel IP: init_accelerator: Accel Interrupt init failed.\n");
            return ACCEL_RET_FAIL_INTR_INIT;
        }
    }
    else
    {
        printf("accel lib: Skipping Accel Interrupt init for SVP\n");
    }

    return ACCEL_RET_SUCCESS;
}

static void update_accel_ctxt_from_knobs(subsystem_ctxt_t* p_ss_ctxt)
{
    DIE_INSTANCE die_id = p_ss_ctxt->accelip_metadata.die_instance;
    sdm_pre_pcie_cfg_t* pre_pcie_cfg = p_ss_ctxt->p_init_params->pre_pcie_cfg;
    accelip_ss_init_knobs_t* ss_cfg = p_ss_ctxt->p_init_params->accelip_ss_cfg;

    if (p_ss_ctxt->accelip_metadata.accel_type == D0_ACCELIP_SDMSS || p_ss_ctxt->accelip_metadata.accel_type == D1_ACCELIP_SDMSS)
    {
        pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_base_class_code =
            config_get_sdm_base_class_code().pf_pci_t0_base_class_code[die_id];
        pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_subsystem_id =
            config_get_sdm_pf_subsystem_id().pf_pci_t0_subsystem_id[die_id];
        pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_device_id = config_get_sdm_pf_device_id().pf_pci_t0_device_id[die_id];
        pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_int_pin = config_get_sdm_pf_int_pin().pf_pci_t0_int_pin[die_id];
        pre_pcie_cfg->rcec_pci_t0_cfg.pci_t0_int_pin = config_get_sdm_rcec_int_pin().rcec_pci_t0_int_pin[die_id];
        pre_pcie_cfg->tee_io_supp = config_get_sdm_tee_io_supp().tee_io_supp[die_id];
        pre_pcie_cfg->dis_pri = config_get_sdm_dis_pri().dis_pri[die_id];
        pre_pcie_cfg->allow_gpa = config_get_sdm_allow_gpa().allow_gpa[die_id];
        pre_pcie_cfg->allow_va = config_get_sdm_allow_va().allow_va[die_id];
        pre_pcie_cfg->allow_cpl_gpa = config_get_sdm_allow_cpl_gpa().allow_cpl_gpa[die_id];
        pre_pcie_cfg->allow_cpl_va = config_get_sdm_allow_cpl_va().allow_cpl_va[die_id];
        pre_pcie_cfg->allow_priv = config_get_sdm_allow_priv().allow_priv[die_id];
        pre_pcie_cfg->allow_nonpriv = config_get_sdm_allow_nonpriv().allow_nonpriv[die_id];
        pre_pcie_cfg->allow_posted = config_get_sdm_allow_posted().allow_posted[die_id];
        pre_pcie_cfg->allow_nonposted = config_get_sdm_allow_nonposted().allow_nonposted[die_id];
        pre_pcie_cfg->gpa_pasid_en = config_get_sdm_gpa_pasid_en().gpa_pasid_en[die_id];
        pre_pcie_cfg->cpl_gpa_pasid_en = config_get_sdm_cpl_gpa_pasid_en().cpl_gpa_pasid_en[die_id];
        pre_pcie_cfg->pf_crs_resp_data = config_get_sdm_pf_crs_resp_data().pf_crs_resp_data[die_id];
        pre_pcie_cfg->vf_crs_resp_data = config_get_sdm_vf_crs_resp_data().vf_crs_resp_data[die_id];
        pre_pcie_cfg->total_vfs = config_get_sdm_total_vfs().total_vfs[die_id];
        pre_pcie_cfg->dti_lim_trans_cnt = config_get_sdm_dti_lim_trans_cnt().dti_lim_trans_cnt[die_id];
        pre_pcie_cfg->dti_inv_lim_trans_cnt = config_get_sdm_dti_inv_lim_trans_cnt().dti_inv_lim_trans_cnt[die_id];
        pre_pcie_cfg->lti_lim_trans_cnt = config_get_sdm_lti_lim_trans_cnt().lti_lim_trans_cnt[die_id];
        pre_pcie_cfg->tot_dwq_num_entries_avail =
            config_get_sdm_tot_dwq_num_entries_avail().tot_dwq_num_entries_avail[die_id];
        pre_pcie_cfg->rd_lim_cnt = config_get_sdm_rd_lim_cnt().rd_lim_cnt[die_id];
        pre_pcie_cfg->wr_lim_cnt = config_get_sdm_wr_lim_cnt().wr_lim_cnt[die_id];

        ss_cfg->isolation_enable = config_get_sdm_isolation_enable().isolation_enable[die_id];
        ss_cfg->emm_resp_code = config_get_sdm_emm_resp_code().emm_resp_code[die_id];
    }
    else
    {
        pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_base_class_code =
            config_get_cded_base_class_code().pf_pci_t0_base_class_code[die_id];
        pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_subsystem_id =
            config_get_cded_pf_subsystem_id().pf_pci_t0_subsystem_id[die_id];
        pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_device_id = config_get_cded_pf_device_id().pf_pci_t0_device_id[die_id];
        pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_int_pin = config_get_cded_pf_int_pin().pf_pci_t0_int_pin[die_id];
        pre_pcie_cfg->rcec_pci_t0_cfg.pci_t0_int_pin = config_get_cded_rcec_int_pin().rcec_pci_t0_int_pin[die_id];
        /*Overwriting the CDED VF device ID*/
        pre_pcie_cfg->sriov_vf_dev_id = CDED_PCI_DEVICE_ID;
        pre_pcie_cfg->tee_io_supp = config_get_cded_tee_io_supp().tee_io_supp[die_id];
        pre_pcie_cfg->dis_wrt_buff_cont = config_get_cded_dis_wrt_buff_cont().dis_wrt_buff_cont[die_id];
        pre_pcie_cfg->dis_pri = config_get_cded_dis_pri().dis_pri[die_id];
        pre_pcie_cfg->allow_gpa = config_get_cded_allow_gpa().allow_gpa[die_id];
        pre_pcie_cfg->allow_va = config_get_cded_allow_va().allow_va[die_id];
        pre_pcie_cfg->allow_cpl_gpa = config_get_cded_allow_cpl_gpa().allow_cpl_gpa[die_id];
        pre_pcie_cfg->allow_cpl_va = config_get_cded_allow_cpl_va().allow_cpl_va[die_id];
        pre_pcie_cfg->allow_priv = config_get_cded_allow_priv().allow_priv[die_id];
        pre_pcie_cfg->allow_nonpriv = config_get_cded_allow_nonpriv().allow_nonpriv[die_id];
        pre_pcie_cfg->allow_posted = config_get_cded_allow_posted().allow_posted[die_id];
        pre_pcie_cfg->allow_nonposted = config_get_cded_allow_nonposted().allow_nonposted[die_id];
        pre_pcie_cfg->gpa_pasid_en = config_get_cded_gpa_pasid_en().gpa_pasid_en[die_id];
        pre_pcie_cfg->cpl_gpa_pasid_en = config_get_cded_cpl_gpa_pasid_en().cpl_gpa_pasid_en[die_id];
        pre_pcie_cfg->pf_crs_resp_data = config_get_cded_pf_crs_resp_data().pf_crs_resp_data[die_id];
        pre_pcie_cfg->vf_crs_resp_data = config_get_cded_vf_crs_resp_data().vf_crs_resp_data[die_id];
        pre_pcie_cfg->total_vfs = config_get_cded_total_vfs().total_vfs[die_id];
        pre_pcie_cfg->dti_lim_trans_cnt = config_get_cded_dti_lim_trans_cnt().dti_lim_trans_cnt[die_id];
        pre_pcie_cfg->dti_inv_lim_trans_cnt = config_get_cded_dti_inv_lim_trans_cnt().dti_inv_lim_trans_cnt[die_id];
        pre_pcie_cfg->lti_lim_trans_cnt = config_get_cded_lti_lim_trans_cnt().lti_lim_trans_cnt[die_id];
        pre_pcie_cfg->tot_dwq_num_entries_avail =
            config_get_cded_tot_dwq_num_entries_avail().tot_dwq_num_entries_avail[die_id];
        pre_pcie_cfg->rd_lim_cnt = config_get_cded_rd_lim_cnt().rd_lim_cnt[die_id];
        pre_pcie_cfg->wr_lim_cnt = config_get_cded_wr_lim_cnt().wr_lim_cnt[die_id];

        ss_cfg->isolation_enable = config_get_cded_isolation_enable().isolation_enable[die_id];
        ss_cfg->emm_resp_code = config_get_cded_emm_resp_code().emm_resp_code[die_id];
    }
}

/*----------------------------- Global Functions ----------------------------*/

ACCEL_ID get_accelip_type(ACCELIP_SS_INSTANCE accel_type)
{
    switch (accel_type)
    {
    case D0_ACCELIP_SDMSS:
        /* FALLTHROUGH */
    case D1_ACCELIP_SDMSS:
        return ACCEL_ID_SDM;
    case D0_ACCELIP_CDEDSS:
        /* FALLTHROUGH */
    case D1_ACCELIP_CDEDSS:
        return ACCEL_ID_CDED;
    default:
        return NUM_VALID_ACCEL_ID;
    }
}

uint32_t accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID accel_type)
{
    return accel_intr_atu_map_address[accel_type];
}

void accel_disable_cpu_wait(ACCEL_ID accel_type)
{
    char accel_name[ACCEL_NAME_LEN];
    uint32_t ext_cfg_offset_addr = get_accel_name_and_offset_addr(accel_type, accel_name);

    sdm_init_disable_cpu_wait((accel_intr_atu_map_address[accel_type] + ext_cfg_offset_addr));
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
            update_accel_ctxt_from_knobs(&p_ss_ctxt[index]);
            ret = init_accelerator(&p_ss_ctxt[index]);

            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);
        }
    }

    return ret;
}

/* TODO: Review use of BUG_ASSERT (vis a vis ASSERT) in accelIP init sequence https://azurecsi.visualstudio.com/Dev/_workitems/edit/2025877/ */
void scp_accelerators_emcpu_reset(ACCEL_ID accel_type, crash_dump_cb_t cb_fun, void* cb_ctx)
{
    uint32_t accel_ctxt_size = 0x0;
    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    char accel_name[ACCEL_NAME_LEN];
    uint32_t ext_cfg_offset_addr = get_accel_name_and_offset_addr(accel_type, accel_name);

    BUG_ASSERT(accel_type < NUM_VALID_ACCEL_ID);
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