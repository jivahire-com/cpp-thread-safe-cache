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
#include "accelerator_ip.h" // for get_accelip_type, scp_accelerators_init
#include "accelerator_ip_events.h"

#include <DbgPrint.h>            // for FPFW_DBGPRINT_INFO
#include <accel_intr.h>          // for accel_scp_intr_init
#include <accelerator_ip_priv.h> // for get_accelerator_ctxt, accel_send_led_boot_status
#include <accelip_id.h>          // for NUM_VALID_ACCEL_ID, ACCEL_ID_SDM, ACCEL_ID_CDED
#include <atu_init.h>            // for atu_svc_accel_atu_addr
#include <boot_status.h>         // for LED_STATUS_CODE_SCP_ACCEL_FAILED
#include <bug_check.h>           // for BUG_ASSERT_PARAM
#include <cdedss_config_regs.h>  // for CDED_PCI_DEVICE_ID
#include <fpfw_cfg_mgr.h>        // for config_get_sdm_*, config_get_cded_*
#include <fpfw_init.h>           // for fpfw_init_get_handle
#include <idsw.h>                // for idsw_get_die_id
#include <kng_icc_shared.h>
#include <kng_soc_constants.h> // for DIE_INSTANCE
#include <silibs_status.h>     // for SILIBS_SUCCESS
#include <stdbool.h>           // for false
#include <stdint.h>            // for int32_t, uint32_t
#include <stdio.h>             // for NULL
#include <system_info.h>       // for system_info_is_hsp_present, system_info_is_warm_start
#include <utils.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

static int32_t init_accelerator(subsystem_ctxt_t* p_ss_ctxt)
{
    int32_t ret = ACCEL_RET_SUCCESS;
    ACCEL_ID accel_type = get_accelip_type(p_ss_ctxt->accelip_metadata.accel_type);

    if (accel_type == NUM_VALID_ACCEL_ID)
    {
        return ACCEL_RET_FAIL_INVALID_PARAMS;
    }

    if (!system_info_is_hsp_present())
    {
        p_ss_ctxt->p_init_params->fw_preload_enabled = false;
    }

    /**
     * Do not initialize the accelerator subsystem in case of SCP warm reboot
     */
    if (!system_info_is_warm_start())
    {
        ret = accelip_ss_init(atu_svc_accel_atu_addr(accel_type),
                              p_ss_ctxt->accelip_metadata.accel_type,
                              p_ss_ctxt->p_init_params);
        if (ret != SILIBS_SUCCESS)
        {
            FPFW_ET_LOG(AccelIPInitError, accel_type, ret, __LINE__);
            return ACCEL_RET_FAIL_SS_INIT;
        }
    }
    else
    {
        // Reset accel here in case of SCP warm boot
        accel_core_suspend(accel_type);
    }

    /*
     * TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/2023638/
     * How do we handle SDM firmware download in SCP warm boot scenario?
     */

    ret = accel_scp_intr_init(accel_type);
    if (ret != ACCEL_INTR_RET_SUCCESS)
    {
        FPFW_ET_LOG(AccelIPInitError, accel_type, ret, __LINE__);
        return ACCEL_RET_FAIL_INTR_INIT;
    }

    return ACCEL_RET_SUCCESS;
}

static PLACED_CODE void update_accel_ctxt_from_knobs(subsystem_ctxt_t* p_ss_ctxt)
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
        /* Overwriting the SDM VF's - Subsystem ID and Device ID */
        pre_pcie_cfg->rciep_pci_t0_vf_cfg.pci_t0_subsystem_id =
            config_get_sdm_pf_subsystem_id().pf_pci_t0_subsystem_id[die_id];
        pre_pcie_cfg->sriov_vf_dev_id = config_get_sdm_pf_subsystem_id().pf_pci_t0_subsystem_id[die_id];
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
        /* Overwriting the CDED VF's - Subsystem ID and Device ID */
        pre_pcie_cfg->rciep_pci_t0_vf_cfg.pci_t0_subsystem_id =
            config_get_cded_pf_subsystem_id().pf_pci_t0_subsystem_id[die_id];
        pre_pcie_cfg->sriov_vf_dev_id = config_get_cded_pf_subsystem_id().pf_pci_t0_subsystem_id[die_id];
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

int32_t scp_accelerators_init(void)
{
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    int ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    if (!accel_is_isolation_enabled(ACCEL_ID_SDM) || !accel_is_isolation_enabled(ACCEL_ID_CDED))
    {
        accel_send_led_boot_status(LED_STATUS_CODE_SCP_ACCEL_INIT_START);
    }

    accel_boot_status_init_sem();

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);
    if (p_ss_ctxt == NULL)
    {
        goto accel_init_failed;
    }

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            if (!accel_is_isolation_enabled(get_accelip_type(p_ss_ctxt[index].accelip_metadata.accel_type)))
            {
                update_accel_ctxt_from_knobs(&p_ss_ctxt[index]);
                ret = init_accelerator(&p_ss_ctxt[index]);
                FPFW_DBGPRINT_INFO("accel lib: Die %d type %d instance %d stat %d\n",
                                   p_ss_ctxt[index].accelip_metadata.die_instance,
                                   p_ss_ctxt[index].accelip_metadata.accel_type,
                                   p_ss_ctxt[index].accelip_metadata.accel_instance,
                                   ret);
                if (ret != ACCEL_RET_SUCCESS)
                {
                    goto accel_init_failed;
                }
            }
            else
            {
                FPFW_DBGPRINT_INFO("Accel type %d on Die %d has been isolated\n",
                                   p_ss_ctxt[index].accelip_metadata.accel_type,
                                   current_die_instance);
            }
        }
    }

    return ret;

accel_init_failed:
    accel_send_led_boot_status(LED_STATUS_CODE_SCP_ACCEL_FAILED);
    BUG_ASSERT(false);
    return ACCEL_RET_FAIL_GENERAL;
}

void notify_accelerators_uefi_boot_cb(void* context, fpfw_status_t status)
{
    ACCEL_ID accel_type = (ACCEL_ID)context;
    if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        FPFW_ET_LOG(AccelIPInitError, accel_type, status, __LINE__);
    }
    else
    {
        FPFW_DBGPRINT_INFO("Accel (%s) UEFI boot notif. success\n", (accel_type == ACCEL_ID_SDM ? "SDM" : "CDED"));
    }
}

int32_t notify_accelerators_uefi_boot(void)
{
    fpfw_icc_base_ctx_t* icc_ctx = {};
    fpfw_status_t icc_status = FPFW_ICC_BASE_STATUS_SUCCESS;
    static uefi_boot_stat_mailbox_msg payload_sdm = {0};
    static uefi_boot_stat_mailbox_msg payload_cded = {0};
    static fpfw_icc_base_send_req_t accel_send_params = {0};
    static fpfw_icc_base_send_req_t accel_send_params_cded = {0};

    for (ACCEL_ID accel_type = ACCEL_ID_SDM; (accel_type < NUM_VALID_ACCEL_ID); accel_type++)
    {
        if (accel_is_isolation_enabled(accel_type))
        {
            FPFW_DBGPRINT_INFO("Accel UEFI boot notif. skipped (isolated)\n");
            return (ACCEL_RET_SUCCESS);
        }

        if (accel_type == ACCEL_ID_SDM)
        {
            icc_ctx = fpfw_init_get_handle("icc_sdm_mbx");
            BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);

            payload_sdm.hdr.cmd = LARGE_FIFO_MAILBOX_MSG_UEFI_READY;

            accel_send_params.payload_buffer = &payload_sdm;
            accel_send_params.buffer_size = sizeof(payload_sdm);
            accel_send_params.cb = notify_accelerators_uefi_boot_cb;
            accel_send_params.cb_ctx = (void*)ACCEL_ID_SDM;
            icc_status = fpfw_icc_base_send(icc_ctx, &accel_send_params);
        }
        else
        {
            icc_ctx = fpfw_init_get_handle("icc_cded_mbx");
            BUG_ASSERT_PARAM(icc_ctx != NULL, icc_ctx, 0);

            payload_cded.hdr.cmd = LARGE_FIFO_MAILBOX_MSG_UEFI_READY;

            accel_send_params_cded.payload_buffer = &payload_cded;
            accel_send_params_cded.buffer_size = sizeof(payload_cded);
            accel_send_params_cded.cb = notify_accelerators_uefi_boot_cb;
            accel_send_params_cded.cb_ctx = (void*)(ACCEL_ID_CDED);
            icc_status = fpfw_icc_base_send(icc_ctx, &accel_send_params_cded);
        }

        if (icc_status != FPFW_ICC_BASE_STATUS_SUCCESS)
        {
            FPFW_ET_LOG(AccelIPInitError, accel_type, icc_status, __LINE__);
            return (ACCEL_RET_FAIL_GENERAL);
        }
    }
    return (ACCEL_RET_SUCCESS);
}
