//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file mesh.c
 *  This modules initializes various Mesh components
 */

/*------------- Includes -----------------*/
#include <FPFwInterrupts.h> // for FPFwCoreInterruptRegisterCallback, FPFwCoreInterruptHandler, FPFwCoreInterruptEnableVector
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <bug_check.h>
#include <cml.h>
#include <cmn800.h>
#include <cmn800_error_handler.h>  // for acpi_err_sec_generic_t
#include <cmn800_sequence.h>       // for cmn800_sequence, d2dss_sequence, cmn8...
#include <cmn800_sequence_knobs.h> // for cmn800_sequence, d2dss_sequence, cmn8...
#include <cmn800_sequence_struct_defaults.h>
#include <cmn_config.h> // for CMN800_CONFIG_CONFIG
#include <d2dss.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_ctx_t
#include <hsp_firmware_headers.h>
#include <i3c_controller.h>
#include <idhw.h>     // for idhw_is_single_die_boot_en
#include <idsw.h>     // for idsw_get_platform_sdv,
#include <idsw_kng.h> // for PLATFORM_FPGA_LARGE
#include <interrupts.h>
#include <kng_soc_constants.h> // for NUM_DIE
#include <mesh.h>              // for mesh_init
#include <mesh_error_handler.h>
#include <mscp_exp_rmss_memory_map.h>
#include <stdbool.h> // for true
#include <stdint.h>  // for uint8_t
#include <stdio.h>   // for MESH_INFO
#include <system_info.h>

/*------------- Defines -----------------*/
static fpfw_icc_base_ctx_t* s_mbx_icc_ctx;
const int MEANINGLESS_NUMBER_MESH = 10;
static int unused_parameter_not_null = MEANINGLESS_NUMBER_MESH;

/*------------- Functions ----------------*/

static void hsp_send_recv_progress_msg(uint16_t req_msg, uint16_t rsp_msg)
{
    size_t recv_msg_size_bytes = 0;
    kng_hsp_mailbox_msg msg = {
        .header.cmd = req_msg,
    };

    // if rsp_msg == HSP_MAILBOX_CMD_MAX, then we don't expect a return from HSP
    if (rsp_msg != HSP_MAILBOX_CMD_MAX)
    {
        //! Send the message to HSP & get response, blocking call
        fpfw_status_t icc_status =
            fpfw_icc_base_send_recv_sync(s_mbx_icc_ctx, &msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);

        //! Verify sync return status & response message
        BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
        BUG_ASSERT(recv_msg_size_bytes > 0);
        BUG_ASSERT(msg.header.cmd == rsp_msg);
        BUG_ASSERT(msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);
    }
    else
    {
        fpfw_status_t icc_status = fpfw_icc_base_send_sync(s_mbx_icc_ctx, &msg, sizeof(kng_hsp_mailbox_msg));
        //! Verify sync return status & response message
        BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
    }
}

void print_mesh_d2d_knob_values(void)
{
    // Print the Mesh SAM Knobs
    cmn800_sam_cfg_t* temp_cmn800_sam_cfg = cmn800_get_mesh_sam_cfg_knob();
    UNUSED(temp_cmn800_sam_cfg);
    MESH_DBG("Print the Mesh SAM Knobs\n");
    MESH_DBG("mesh_hnf_cbusy_limit_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_cbusy_limit_ctl));
    MESH_DBG("mesh_hnf_cbusy_resp_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_cbusy_resp_ctl));
    MESH_DBG("mesh_hnf_cbusy_sn_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_cbusy_sn_ctl));
    MESH_DBG("mesh_hnf_cbusy_write_limit_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_cbusy_write_limit_ctl));
    MESH_DBG("mesh_hnf_cbusy_mode_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_cbusy_mode_ctl));
    MESH_DBG("mesh_hnf_aux_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_aux_ctl));
    MESH_DBG("mesh_hnf_r2_aux_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_r2_aux_ctl));
    MESH_DBG("mesh_hnf_cfg_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_cfg_ctl));
    MESH_DBG("mesh_hnf_lbt_cfg_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_lbt_cfg_ctl));
    MESH_DBG("mesh_hnf_lbt_aux_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_lbt_aux_ctl));
    MESH_DBG("mesh_hnf_lbt_cbusy_ctl = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnf_lbt_cbusy_ctl));
    for (int i = 0; i < 6; i++)
    {
        MESH_DBG("mesh_hni_cfg_ctl[%d] = 0x%x_%x\n", i, PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hni_cfg_ctl[i]));
    }
    for (int i = 0; i < 6; i++)
    {
        MESH_DBG("mesh_hni_aux_ctl[%d] = 0x%x_%x\n", i, PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hni_aux_ctl[i]));
    }
    MESH_DBG("mesh_hnt_dn_domain_cxra = 0x%x_%x\n", PRINT64_HEX(temp_cmn800_sam_cfg->mesh_hnt_dn_domain_cxra));
    for (int i = 0; i < 8; i++)
    {
        MESH_DBG("mesh_rni_cfg_ctl[%d] = 0x%x_%x\n", i, PRINT64_HEX(temp_cmn800_sam_cfg->mesh_rni_cfg_ctl[i]));
    }
    for (int i = 0; i < 8; i++)
    {
        MESH_DBG("mesh_rni_aux_ctl[%d] = 0x%x_%x\n", i, PRINT64_HEX(temp_cmn800_sam_cfg->mesh_rni_aux_ctl[i]));
    }
    for (int i = 0; i < 7; i++)
    {
        MESH_DBG("mesh_rnd_cfg_ctl[%d] = 0x%x_%x\n", i, PRINT64_HEX(temp_cmn800_sam_cfg->mesh_rnd_cfg_ctl[i]));
    }
    for (int i = 0; i < 7; i++)
    {
        MESH_DBG("mesh_rnd_aux_ctl[%d] = 0x%x_%x\n", i, PRINT64_HEX(temp_cmn800_sam_cfg->mesh_rnd_aux_ctl[i]));
    }

    // Print the Mesh RAS Knobs
    cmn800_ras_cfg_t* temp_cmn800_ras_cfg = cmn800_get_mesh_ras_cfg_knob();
    UNUSED(temp_cmn800_ras_cfg);
    MESH_DBG("Print the Mesh RAS Knobs\n");
    MESH_DBG("mesh_RAS_Error_Detection_Disable = %d\n", temp_cmn800_ras_cfg->mesh_RAS_Error_Detection_Disable);
    MESH_DBG("mesh_RAS_Error_Deferment_Disable = %d\n", temp_cmn800_ras_cfg->mesh_RAS_Error_Deferment_Disable);
    MESH_DBG("mesh_RAS_Uncorrected_Error_Int_Disable = %d\n", temp_cmn800_ras_cfg->mesh_RAS_Uncorrected_Error_Int_Disable);
    MESH_DBG("mesh_RAS_Fault_Handling_Int_Disable = %d\n", temp_cmn800_ras_cfg->mesh_RAS_Fault_Handling_Int_Disable);
    MESH_DBG("mesh_RAS_Corrected_Error_Int_Disable = %d\n", temp_cmn800_ras_cfg->mesh_RAS_Corrected_Error_Int_Disable);
    MESH_DBG("mesh_RAS_Parity_Error_Disable = %d\n", temp_cmn800_ras_cfg->mesh_RAS_Parity_Error_Disable);

    // Print the Mesh CCG Knobs
    ccg_cfg_t* temp_ccg_cfg = get_ccg_knob_defaults();
    UNUSED(temp_ccg_cfg);
    MESH_DBG("Print the Mesh CCG Knobs\n");
    MESH_DBG("por_ccg_ha_aux_ctl = 0x%x_%x\n", PRINT64_HEX(temp_ccg_cfg->por_ccg_ha_aux_ctl));
    MESH_DBG("por_ccg_ha_cfg_ctl = 0x%x_%x\n", PRINT64_HEX(temp_ccg_cfg->por_ccg_ha_cfg_ctl));
    MESH_DBG("por_ccg_ha_cxprtcl_link0_ctl = 0x%x_%x\n", PRINT64_HEX(temp_ccg_cfg->por_ccg_ha_cxprtcl_link0_ctl));
    MESH_DBG("por_ccg_ra_cfg_ctl = 0x%x_%x\n", PRINT64_HEX(temp_ccg_cfg->por_ccg_ra_cfg_ctl));
    MESH_DBG("por_ccg_ra_aux_ctl = 0x%x_%x\n", PRINT64_HEX(temp_ccg_cfg->por_ccg_ra_aux_ctl));
    MESH_DBG("por_ccg_ra_ccprtcl_link0_ctl = 0x%x_%x\n", PRINT64_HEX(temp_ccg_cfg->por_ccg_ra_ccprtcl_link0_ctl));
    MESH_DBG("por_ccg_ra_cbusy_limit_ctl = 0x%x_%x\n", PRINT64_HEX(temp_ccg_cfg->por_ccg_ra_cbusy_limit_ctl));
    MESH_DBG("por_ccla_aux_ctl = 0x%x_%x\n", PRINT64_HEX(temp_ccg_cfg->por_ccla_aux_ctl));

    // Print the Mesh D2D Knobs
    d2d_cfg_t* temp_d2d_cfg = get_default_d2d_cfg();
    UNUSED(temp_d2d_cfg);
    MESH_DBG("Print the Mesh D2D Knobs\n");
    MESH_DBG("d2d_pll_divder = 0x%x\n", temp_d2d_cfg->d2d_pll_divder);
    MESH_DBG("d2d_ref_divder = 0x%x\n", temp_d2d_cfg->d2d_ref_divder);
    MESH_DBG("d2d_pll_fb_devider = 0x%x\n", temp_d2d_cfg->d2d_pll_fb_devider);
    MESH_DBG("d2d_ecc_cfg = 0x%x\n", temp_d2d_cfg->d2d_ecc_cfg);
    MESH_DBG("d2d_tx_interface_clk_alignment = 0x%x\n", temp_d2d_cfg->d2d_tx_interface_clk_alignment);
    MESH_DBG("d2d_ras_enable = 0x%x\n", temp_d2d_cfg->d2d_ras_enable);
    MESH_DBG("d2d_sleep_cfg = 0x%x\n", temp_d2d_cfg->d2d_sleep_cfg);
    MESH_DBG("d2d_sleep_cfg_entry = 0x%x\n", temp_d2d_cfg->d2d_sleep_cfg_entry);
    MESH_DBG("d2d_rxcal_find_goodlanes_skip = 0x%x\n", temp_d2d_cfg->d2d_rxcal_find_goodlanes_skip);
}

void mesh_read_cfg_knobs_from_spi(cmn800_sequence_params_t* cmn800_sequence_param)
{
    cmn800_sequence_param->die_num = (uint8_t)idhw_get_die_id();
    cmn800_sequence_param->cmn_config_enum = CONFIG_1D_UMA_64HNS_HIER_3SN_enum;
    cmn800_sequence_param->CMN_INIT = 2; // CMN_INIT_FRONTDOOR_HNS_PPU_POLL_CMN_SAM_CFG
    cmn800_sequence_param->DRAM_SIZE = DRAM_32GB;
    cmn800_sequence_param->BOOT_2D_ENABLE = 0;
    cmn800_sequence_param->HNS_SPARE_DIE0 = 0; // CMN_HNS_SPARE_CONFIG0_X_X_X_X
    cmn800_sequence_param->HNS_SPARE_DIE1 = 0; // CMN_HNS_SPARE_CONFIG0_X_X_X_X
    cmn800_sequence_param->D2D_INIT = 0;       // D2D_INIT_FRONTDOOR
    cmn800_sequence_param->USE_HW_TIMER = 0;
    // cmn800_sequence_param->cmn800_sequence_knobs = DEFAULT_CMN800_SEQUENCE_CFG_T;
    if (!idhw_is_single_die_boot_en()) // 2 Die
    {
        MESH_INFO("Dual Die Boot\n");
        cmn800_sequence_param->cmn_config_enum = CONFIG_2D_NUMA_64HNS_HIER_3SN_enum; // 2 Die
        cmn800_sequence_param->BOOT_2D_ENABLE = true;
    }
    if (idsw_get_platform_sdv() == PLATFORM_FPGA_LARGE || idsw_get_platform_sdv() == PLATFORM_FPGA_LARGE_RVP)
    {
        if (idhw_is_single_die_boot_en())
        {
            cmn800_sequence_param->cmn_config_enum = CONFIG_1D_UMA_8HNS_HIER_3SN_enum; // 1 Die FPGA
        }
        else
        {
            cmn800_sequence_param->cmn_config_enum = CONFIG_2D_NUMA_8HNS_HIER_3SN_enum; // 2 Die FPGA
            cmn800_sequence_param->BOOT_2D_ENABLE = true;
        }
    }
    cmn800_sequence_param->cmn800_sequence_knobs.cmn_sam_config = cmn800_sequence_param->cmn_config_enum;
    // Read the Config Knobs from SPI
    if (config_get_mesh_d2d_override() == true) // Check if the override is enabled
    {
        MESH_DBG("Mesh and D2D User Config Enabled\n");
        if (idsw_get_platform_sdv() != PLATFORM_RVP_EVT_SILICON)
        {
            MESH_DBG("Pre-Si Platform !!\n");
            uint8_t temp8 = config_get_cmn_sam_config();
            UNUSED(temp8);
            MESH_DBG("User knob read cmn_sam_config = 0x%x\n", temp8);
            MESH_DBG("cmn_sam_config not applied for Pre-Si Platform !!\n");
        }
        else
        {
            cmn800_sequence_param->cmn_config_enum = config_get_cmn_sam_config();
            cmn800_sequence_param->cmn800_sequence_knobs.cmn_sam_config = config_get_cmn_sam_config();
        }
        cmn800_sequence_param->cmn800_sequence_knobs.cmn_cxl_interleave = config_get_cmn_cxl_interleave();
        cmn800_sequence_param->cmn800_sequence_knobs.cmn_cxl_device_interleave_size =
            config_get_cmn_cxl_device_interleave_size();
        cmn800_sequence_param->cmn800_sequence_knobs.cmn_uma_arsm_htg_wa_disabled =
            config_get_cmn_uma_arsm_htg_wa_disabled();

        if ((cmn800_sequence_param->cmn_config_enum == CONFIG_2D_NUMA_64HNS_HIER_3SN_enum) ||
            (cmn800_sequence_param->cmn_config_enum == CONFIG_2D_NUMA_8HNS_HIER_3SN_enum))
        {
            cmn800_sequence_param->BOOT_2D_ENABLE = true;
        }

        // Get the default values in Mesh Lib
        cmn800_sam_cfg_t* temp_cmn800_sam_cfg = cmn800_get_mesh_sam_cfg_knob(); // This is the default struct from Mesh Lib
        // Update the values based on the User Config
        temp_cmn800_sam_cfg->mesh_hnf_cbusy_limit_ctl = config_get_mesh_hnf_cbusy_limit_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_cbusy_resp_ctl = config_get_mesh_hnf_cbusy_resp_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_cbusy_sn_ctl = config_get_mesh_hnf_cbusy_sn_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_cbusy_write_limit_ctl = config_get_mesh_hnf_cbusy_write_limit_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_cbusy_mode_ctl = config_get_mesh_hnf_cbusy_mode_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_aux_ctl = config_get_mesh_hnf_aux_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_r2_aux_ctl = config_get_mesh_hnf_r2_aux_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_cfg_ctl = config_get_mesh_hnf_cfg_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_lbt_cfg_ctl = config_get_mesh_hnf_lbt_cfg_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_lbt_aux_ctl = config_get_mesh_hnf_lbt_aux_ctl();
        temp_cmn800_sam_cfg->mesh_hnf_lbt_cbusy_ctl = config_get_mesh_hnf_lbt_cbusy_ctl();
        mesh_hni_cfg_ctl_t temp_mesh_hni_cfg_ctl = config_get_mesh_hni_cfg_ctl_knob();
        for (int i = 0; i < 6; i++)
        {
            temp_cmn800_sam_cfg->mesh_hni_cfg_ctl[i] = temp_mesh_hni_cfg_ctl.mesh_hni_cfg_ctl[i];
        }
        mesh_hni_aux_ctl_t temp_mesh_hni_aux_ctl = config_get_mesh_hni_aux_ctl_knob();
        for (int i = 0; i < 6; i++)
        {
            temp_cmn800_sam_cfg->mesh_hni_aux_ctl[i] = temp_mesh_hni_aux_ctl.mesh_hni_aux_ctl[i];
        }
        temp_cmn800_sam_cfg->mesh_hnt_dn_domain_cxra = config_get_mesh_hnt_dn_domain_cxra();
        mesh_rni_cfg_ctl_t temp_mesh_rni_cfg_ctl = config_get_mesh_rni_cfg_ctl_knob();
        for (int i = 0; i < 8; i++)
        {
            temp_cmn800_sam_cfg->mesh_rni_cfg_ctl[i] = temp_mesh_rni_cfg_ctl.mesh_rni_cfg_ctl[i];
        }
        mesh_rni_aux_ctl_t temp_mesh_rni_aux_ctl = config_get_mesh_rni_aux_ctl_knob();
        for (int i = 0; i < 8; i++)
        {
            temp_cmn800_sam_cfg->mesh_rni_aux_ctl[i] = temp_mesh_rni_aux_ctl.mesh_rni_aux_ctl[i];
        }
        mesh_rnd_cfg_ctl_t temp_mesh_rnd_cfg_ctl = config_get_mesh_rnd_cfg_ctl_knob();
        for (int i = 0; i < 7; i++)
        {
            temp_cmn800_sam_cfg->mesh_rnd_cfg_ctl[i] = temp_mesh_rnd_cfg_ctl.mesh_rnd_cfg_ctl[i];
        }
        mesh_rnd_aux_ctl_t temp_mesh_rnd_aux_ctl = config_get_mesh_rnd_aux_ctl_knob();
        for (int i = 0; i < 7; i++)
        {
            temp_cmn800_sam_cfg->mesh_rnd_aux_ctl[i] = temp_mesh_rnd_aux_ctl.mesh_rnd_aux_ctl[i];
        }

        // Read and Update the Mesh RAS Knobs
        cmn800_ras_cfg_t* temp_cmn800_ras_cfg = cmn800_get_mesh_ras_cfg_knob();
        temp_cmn800_ras_cfg->mesh_RAS_Error_Detection_Disable = config_get_mesh_RAS_Error_Detection_Disable();
        temp_cmn800_ras_cfg->mesh_RAS_Error_Deferment_Disable = config_get_mesh_RAS_Error_Deferment_Disable();
        temp_cmn800_ras_cfg->mesh_RAS_Uncorrected_Error_Int_Disable =
            config_get_mesh_RAS_Uncorrected_Error_Int_Disable();
        temp_cmn800_ras_cfg->mesh_RAS_Fault_Handling_Int_Disable = config_get_mesh_RAS_Fault_Handling_Int_Disable();
        temp_cmn800_ras_cfg->mesh_RAS_Corrected_Error_Int_Disable = config_get_mesh_RAS_Corrected_Error_Int_Disable();
        temp_cmn800_ras_cfg->mesh_RAS_Parity_Error_Disable = config_get_mesh_RAS_Parity_Error_Disable();

        // Read and Update the CML Knobs
        ccg_cfg_t* temp_ccg_cfg = get_ccg_knob_defaults();
        temp_ccg_cfg->por_ccg_ha_aux_ctl = config_get_por_ccg_ha_aux_ctl();
        temp_ccg_cfg->por_ccg_ha_cfg_ctl = config_get_por_ccg_ha_cfg_ctl();
        temp_ccg_cfg->por_ccg_ha_cxprtcl_link0_ctl = config_get_por_ccg_ha_cxprtcl_link0_ctl();
        temp_ccg_cfg->por_ccg_ra_cfg_ctl = config_get_por_ccg_ra_cfg_ctl();
        temp_ccg_cfg->por_ccg_ra_aux_ctl = config_get_por_ccg_ra_aux_ctl();
        temp_ccg_cfg->por_ccg_ra_ccprtcl_link0_ctl = config_get_por_ccg_ra_ccprtcl_link0_ctl();
        temp_ccg_cfg->por_ccg_ra_cbusy_limit_ctl = config_get_por_ccg_ra_cbusy_limit_ctl();
        temp_ccg_cfg->por_ccla_aux_ctl = config_get_por_ccla_aux_ctl();

        // Read and Update the D2D Knobs
        d2d_cfg_t* temp_d2d_cfg = get_default_d2d_cfg();
        temp_d2d_cfg->d2d_pll_divder = config_get_d2d_pll_divder();
        temp_d2d_cfg->d2d_ref_divder = config_get_d2d_ref_divder();
        temp_d2d_cfg->d2d_pll_fb_devider = config_get_d2d_pll_fb_devider();
        temp_d2d_cfg->d2d_ecc_cfg = config_get_d2d_ecc_cfg();
        temp_d2d_cfg->d2d_tx_interface_clk_alignment = config_get_d2d_tx_interface_clk_alignment();
        temp_d2d_cfg->d2d_ras_enable = config_get_d2d_ras_enable();
        temp_d2d_cfg->d2d_sleep_cfg = config_get_d2d_sleep_cfg();
        temp_d2d_cfg->d2d_sleep_cfg_entry = config_get_d2d_sleep_cfg_entry();
        temp_d2d_cfg->d2d_rxcal_find_goodlanes_skip = config_get_d2d_rxcal_find_goodlanes_skip();

        MESH_DBG("Mesh and D2D Knob values after cfg_mgr read\n");
        print_mesh_d2d_knob_values();
    }
    else
    {
        MESH_INFO("Mesh and D2D User Config Disabled\n");
    }
}

int mesh_read_binary_table_from_rmss(uint8_t die_num, uint32_t cmn_config_enum)
{
    int sts = SILIBS_SUCCESS;
    // Read the Binary Table from SPI
    MESH_CRIT("Mesh Binary Table Read from RMSS Start, die_num %d, CMN Config 0x%x\n", die_num, cmn_config_enum);
    MESH_DBG("SCP_EXP_MESH_BIN_DATA_BASE 0x%x\n", SCP_EXP_MESH_BIN_DATA_BASE);
    MESH_DBG("SCP_EXP_MESH_BIN_DATA_SIZE 0x%x\n", SCP_EXP_MESH_BIN_DATA_SIZE);

    uintptr_t mesh_binary_base = SCP_EXP_MESH_BIN_DATA_BASE;

    sts = process_mesh_binary_from_spi(mesh_binary_base, cmn_config_enum);
    if (sts != SILIBS_SUCCESS)
    {
        MESH_CRIT("process_mesh_binary_from_spi failed sts 0x%x\n", sts);
    }
    MESH_CRIT("Mesh Bin Tbl Rd RMSS End sts 0x%x\n", sts);
    return sts;
}

void mesh_init(uint8_t die_num, fpfw_icc_base_ctx_t* icc_ctx)
{
    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);

    s_mbx_icc_ctx = icc_ctx;

    int sts = 0x0;

    MESH_CRIT("mesh_init Cold start Flow\n");

    cmn800_sequence_params_t cmn800_sequence_param = {};

    mesh_read_cfg_knobs_from_spi(&cmn800_sequence_param);

    if (is_i3c_supported())
    {
        uint8_t dimm_sku = get_i3c_dimm_sku();
        MESH_INFO("dimm_sku 0x%x\n", dimm_sku);
        uint32_t ddrss_mask = get_i3c_dimm_detected();
        MESH_INFO("ddrss_mask 0x%x\n", (unsigned int)ddrss_mask);
        cmn800_set_i3c_dimm_params(dimm_sku, ddrss_mask);
    }

    MESH_INFO("cmn800_sequence_param.cmn_config_enum 0x%x\n", (uint8_t)cmn800_sequence_param.cmn_config_enum);

    sts = mesh_read_binary_table_from_rmss(die_num, cmn800_sequence_param.cmn_config_enum);
    if (sts != 0)
    {
        MESH_CRIT("mesh_read_binary_table_from_rmss failed sts 0x%x\n", sts);
    }

    if (system_info_is_warm_start())
    {
        MESH_CRIT("mesh_init Warm restart Flow\n");

        // Call to re-init the D2D RAS Agents (API to be added)

        return;
    }

    sts = cmn800_sequence_svp_updates(cmn800_sequence_param);
    MESH_INFO("cmn800_sequence_svp_updates sts 0x%x\n", sts);
    FPFW_RUNTIME_ASSERT(sts == 0);

    sts = cmn800_sequence(cmn800_sequence_param);
    MESH_INFO("cmn800_sequence sts 0x%x\n", sts);
    FPFW_RUNTIME_ASSERT(sts == 0);

    if (system_info_is_hsp_present())
    {
        MESH_INFO("Send enable SMMU in bypass mode\n");
        hsp_send_recv_progress_msg(HSP_MAILBOX_CMD_ENABLE_SMMU_ACCESS_REQ, HSP_MAILBOX_CMD_ENABLE_SMMU_ACCESS_RSP);
    }

    if (cmn800_sequence_param.BOOT_2D_ENABLE)
    {
        sts = d2dss_sequence(cmn800_sequence_param);
        MESH_INFO("d2dss_sequence sts 0x%x\n", sts);
        FPFW_RUNTIME_ASSERT(sts == 0);
        // Setup the D2D RAS Agents for Interrupt Handling
        d2d_ras_init();
    }
    else
    {
        MESH_INFO("Skip d2dss_sequence\n");
    }

    uint32_t intr_status = 0;
    intr_status = FPFwCoreInterruptRegisterCallback(HW_INT_INTREQFAULTS,
                                                    (FPFwCoreInterruptHandler)mesh_fault_isr,
                                                    (void*)&unused_parameter_not_null);
    intr_status |= FPFwCoreInterruptEnableVector(HW_INT_INTREQFAULTS);
    FPFW_RUNTIME_ASSERT(intr_status == 0);

    intr_status = FPFwCoreInterruptRegisterCallback(HW_INT_INTREQERRS,
                                                    (FPFwCoreInterruptHandler)mesh_error_isr,
                                                    (void*)&unused_parameter_not_null);
    intr_status |= FPFwCoreInterruptEnableVector(HW_INT_INTREQERRS);
    FPFW_RUNTIME_ASSERT(intr_status == 0);

    intr_status = FPFwCoreInterruptRegisterCallback(HW_INT_INTREQFAULTNS,
                                                    (FPFwCoreInterruptHandler)mesh_ns_fault_isr,
                                                    (void*)&unused_parameter_not_null);
    intr_status |= FPFwCoreInterruptEnableVector(HW_INT_INTREQFAULTNS);
    FPFW_RUNTIME_ASSERT(intr_status == 0);

    intr_status = FPFwCoreInterruptRegisterCallback(HW_INT_INTREQERRNS,
                                                    (FPFwCoreInterruptHandler)mesh_ns_error_isr,
                                                    (void*)&unused_parameter_not_null);
    intr_status |= FPFwCoreInterruptEnableVector(HW_INT_INTREQERRNS);
    FPFW_RUNTIME_ASSERT(intr_status == 0);

    MESH_CRIT("Mesh Init Done\n");

    // ADO 1513835: The INT HW_INT_VAB4_COMBINED_SCP_INT ISR and Vector needs to be enabled as part of INTU combined

    if (system_info_is_hsp_present())
    {
        MESH_INFO("Send CML Ready Notify\n");
        hsp_send_recv_progress_msg(HSP_MAILBOX_CMD_CML_READY_NOTIFY, HSP_MAILBOX_CMD_MAX);
        MESH_INFO("Send CML Ready Notify Done\n");
    }
}
