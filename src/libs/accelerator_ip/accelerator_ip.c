//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip.c
 * This file provides interface to initializes the Accelerator IP(s) available
 * on the SoC.
 */

/*-------------------------------- Features ---------------------------------*/
/*
 * TODO: ADO 1831262: Remove the below flag and related code once HSP supports
 * initialization of CDEDSS Tower.
 */
#define FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP

/*-------------------------------- Includes ---------------------------------*/
#include "accelerator_ip.h"

#include "accelerator_ip_pcie_params.h" // for pcie_type0_ctxt_t, pcie_ctr...
#include "sdm_ext_cfg_regs.h"           // for ptr_sdm_ext_cfg_reg, (anony...
#include "sdm_init_knobs.h"             // for sdm_pre_pcie_cfg_t

#include <FpFwAssert.h>                  // for FPFW_RUNTIME_ASSERT
#include <_addressblock_0x100000_regs.h> // for _addressblock_0x100000_bcfg...
#include <accelerator_ip_priv.h>         // for get_accelerator_ctxt
#include <atu_lib.h>                     // for atu_map, atu_unmap, atu_map...
#include <cdedss_config_regs.h>          // for CDEDSS_CONFIG_CDEDSS_PCR_AD...
#include <idsw.h>                        // for idsw_get_platform_sdv, idsw...
#include <idsw_kng.h>                    // for PLATFORM_SVP_SIM
#include <kng_soc_constants.h>           // for DIE_INSTANCE
#include <pcr_clock_config.h>            // for PCR_CLOCK_SELECT_B
#include <pcr_rpss.h>                    // for pcr_rpss_configure_clock
#include <sdm_init.h>                    // for sdm_init_enable_ecam, sdm_i...
#include <sdmss_config_regs.h>           // for SDMSS_CONFIG_SDMSS_PCR_TOP_...
#include <silibs_kng_soc.h>              // for PCIE_ECAM_START
#include <silibs_platform.h>             // for debug_print, MMIO_UPDATE32
#include <silibs_status.h>               // for SILIBS_SUCCESS
#include <stdbool.h>                     // for true
#include <stdint.h>                      // for int32_t, uintptr_t, uint32_t
#include <stdio.h>                       // for printf, NULL
#include <utils.h>                       // for UNUSED

#if defined(FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP)
    #include <silibs_common.h> // for ALIGN_UP
    #include <tower_cdedss.h>  // for configure_cdedss_hsp_system_addr_map()
#endif                         /* FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP */

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

#define ACCEL_PCR_CONFIG_TIMEOUT (10)

#define SLEEP_100_MS                                (100)
#define MAX_RETRY_CNT_FOR_SMMU_CONFIGURE_GPBA_CHECK (50)

#if defined(FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP)
    #define HSSS_DFP_TOP_KD_CDEDSS_HSP_AXI_ADDRESS (0xffffff0000000U)
#endif /* FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP */

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/
static int32_t accelss_init_pcr(uint32_t accelss_pcr_base_addr)
{
    pcr_rpss_entity_t pcr = {0};
    int sts = ACCEL_RET_SUCCESS;
    sts = pcr_rpss_init_entity(&pcr, PCR_RPSS_CLOCK_EN_APB | PCR_RPSS_CLOCK_EN_P0, accelss_pcr_base_addr);
    ASSERT_FAIL(sts == SILIBS_SUCCESS);
    sts = pcr_rpss_configure_clock(&pcr, PCR_RPSS_CLOCK_APB, PCR_CLOCK_SELECT_B, PCR_RPSS_DEFAULT_DIV, ACCEL_PCR_CONFIG_TIMEOUT);
    ASSERT_FAIL(sts == SILIBS_SUCCESS);
    sts = pcr_rpss_configure_clock(&pcr, PCR_RPSS_CLOCK_P0, PCR_CLOCK_SELECT_B, PCR_RPSS_NO_DIV, ACCEL_PCR_CONFIG_TIMEOUT);
    ASSERT_FAIL(sts == SILIBS_SUCCESS);
    sts = pcr_rpss_deassert_por_reset(&pcr);
    ASSERT_FAIL(sts == SILIBS_SUCCESS);

    return sts;
}

static int32_t configure_accel_subsystem_pcr(accelip_metadata_t accelip_metadata,
                                             tower_attr_t* p_accelss_tower_attr,
                                             atu_mapping_ctxt_t* p_accelss_atu_mapping_ctxt)
{
    atu_map_entry_t* p_atu_map_entry = p_accelss_atu_mapping_ctxt->p_atu_map_entry;
    atu_id_t atu_id = p_accelss_atu_mapping_ctxt->ss_atu_id;
    int32_t sts = SILIBS_SUCCESS;

    // Will be used when more IP of this tower will be configured.
    UNUSED(p_accelss_tower_attr);

    debug_print("Accel SS PCR Init start\n");

    // Create ATU Mapping (SCP View) for the Accel SS
    int32_t ret = atu_map(atu_id, p_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Accel SS ATU Mapping failed\n");
        return ACCEL_RET_FAIL_ATU_MAP;
    }

    // Get Accel subsystem base address and tower base address after ATU mapped successfully
    uint32_t mscp_start_address = p_atu_map_entry->mscp_start_address;
    uint32_t accelss_pcr_base_addr = 0;

    if (accelip_metadata.accel_type == ACCELERATOR_SDMSS)
    {
        // update the PCR Base for SDMSS
        accelss_pcr_base_addr = mscp_start_address + SDMSS_CONFIG_SDMSS_PCR_TOP_ADDRESS;
    }
    else // accelip_metadata.accel_type == ACCELERATOR_CDEDSS
    {
        /*
         * Since HSP FW is not ready for the handshake, we would be consuming
         * the cmm scripts from FPGA team to configure the CDEDSS Tower.
         *
         * Once HSP FW is available, we need to follow the below steps:
         *   - Send request to HSP to configure the CDEDSS Tower via Mailbox.
         *   - Wait for the confirmation from HSP via Mailbox about the status
         *     to configure the CDEDSS Tower.
         *   - Take further action based on SUCCESS/FAIL.
         *
         * This ADO catpures the status of HSP FW readiness: 1568266
         */

        // update the PCR Base for SDMSS
        accelss_pcr_base_addr = mscp_start_address + CDEDSS_CONFIG_CDEDSS_PCR_ADDRESS;
    }

    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        // Configure Accel SS PCR
        ret = accelss_init_pcr(accelss_pcr_base_addr);
        if (ret != SILIBS_SUCCESS)
        {
            debug_print("Accel SS PCR configuration failed\n");
            ret = ACCEL_RET_FAIL_SS_PCR;
            goto exit_atu_unmap;
        }
    }

    debug_print("Accel SS PCR Init done\n");

exit_atu_unmap:
    // Destroy ATU Mapping (SCP View) for the given VAB instance
    sts = atu_unmap(atu_id, p_atu_map_entry);
    if (sts != SILIBS_SUCCESS)
    {
        debug_print("Accel SS ATU Unmapping failed\n");
    }

    return ((ret != SILIBS_SUCCESS) ? ret : sts);
}

static void silibs_configure_smmu_connection(uint64_t accelip_config_base_addr)
{
    sdm_init_smmu_connection_seq(accelip_config_base_addr);
}

static int32_t silib_sdm_init_set_pf_type0_ven_dev_id(uintptr_t ext_cfg_addr, uint16_t ven_id, uint16_t dev_id)
{
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    _addressblock_0x100000_bcfg_boot_cfg_pf_type0_sdm_id data = {
        .ven_id = ven_id,
        .dev_id = dev_id,
    };

    MMIO_WRITE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_pf_type0_sdm_id, data.as_uint32);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_pf_type0_subsystem_id(uintptr_t ext_cfg_addr, uint16_t ss_ven_id, uint16_t ss_id)
{
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    _addressblock_0x100000_bcfg_boot_cfg_pf_type0_ss_id data = {
        .subsys_vendor_id = ss_ven_id,
        .subsys_id = ss_id,
    };

    MMIO_WRITE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_pf_type0_ss_id, data.as_uint32);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_pf_type0_rev_id(uintptr_t ext_cfg_addr, uint8_t rev_id)
{
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    MMIO_UPDATE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_pf_type0_rev,
                  rev_id,
                  _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_REV_REV_ID_MASK);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_pf_type0_class_code(uintptr_t ext_cfg_addr, uint8_t prog_intf, uint8_t sub_class_code, uint8_t base_class_code)
{
    uint32_t mask;
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    _addressblock_0x100000_bcfg_boot_cfg_pf_type0_class_code class_code = {
        .prog_intf = prog_intf,
        .sub_class_code = sub_class_code,
        .base_class_code = base_class_code,
    };

    mask = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_PROG_INTF_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_SUB_CLASS_CODE_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_BASE_CLASS_CODE_MASK;

    MMIO_UPDATE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_pf_type0_class_code, class_code.as_uint32, mask);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_pf_type0_intr_pin(uintptr_t ext_cfg_addr, uint8_t intr_pin)
{
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    MMIO_UPDATE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_pf_type0_intr,
                  intr_pin,
                  _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_PF_TYPE0_INTR_PIN_MASK);
    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_rcec_pf_type0_ven_dev_id(uintptr_t ext_cfg_addr, uint16_t ven_id, uint16_t dev_id)
{
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    _addressblock_0x100000_bcfg_boot_cfg_rcec_type0_sdm_id data = {
        .ven_id = ven_id,
        .dev_id = dev_id,
    };

    MMIO_WRITE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_rcec_type0_sdm_id, data.as_uint32);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_rcec_pf_type0_subsystem_id(uintptr_t ext_cfg_addr, uint16_t ss_ven_id, uint16_t ss_id)
{
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    _addressblock_0x100000_bcfg_boot_cfg_rcec_type0_ss_id data = {
        .subsys_vendor_id = ss_ven_id,
        .subsys_id = ss_id,
    };

    MMIO_WRITE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_rcec_type0_ss_id, data.as_uint32);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_rcec_pf_type0_rev_id(uintptr_t ext_cfg_addr, uint8_t rev_id)
{
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    MMIO_UPDATE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_rcec_type0_rev,
                  rev_id,
                  _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_REV_REV_ID_MASK);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_rcec_pf_type0_class_code(uintptr_t ext_cfg_addr, uint8_t prog_intf, uint8_t sub_class_code, uint8_t base_class_code)
{
    uint32_t mask;
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    _addressblock_0x100000_bcfg_boot_cfg_rcec_type0_class_code class_code = {
        .prog_intf = prog_intf,
        .sub_class_code = sub_class_code,
        .base_class_code = base_class_code,
    };

    mask = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_PROG_INTF_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_SUB_CLASS_CODE_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_BASE_CLASS_CODE_MASK;

    MMIO_UPDATE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_rcec_type0_class_code, class_code.as_uint32, mask);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_rcec_pf_type0_intr_pin(uintptr_t ext_cfg_addr, uint8_t intr_pin)
{
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    MMIO_UPDATE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_rcec_type0_intr,
                  intr_pin,
                  _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_RCEC_TYPE0_INTR_PIN_MASK);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_rciep_pf_type0(uintptr_t accelip_config_base_addr, pcie_type0_ctxt_t* p_rciep_ctxt)
{
    silib_sdm_init_set_pf_type0_ven_dev_id(accelip_config_base_addr, p_rciep_ctxt->vendor_id, p_rciep_ctxt->device_id);
    silib_sdm_init_set_pf_type0_subsystem_id(accelip_config_base_addr,
                                             p_rciep_ctxt->ss_vendor_id,
                                             p_rciep_ctxt->subsystem_id);
    silib_sdm_init_set_pf_type0_rev_id(accelip_config_base_addr, p_rciep_ctxt->revision_id);
    silib_sdm_init_set_pf_type0_class_code(accelip_config_base_addr,
                                           p_rciep_ctxt->class_code->prog_intf,
                                           p_rciep_ctxt->class_code->sub_class_code,
                                           p_rciep_ctxt->class_code->base_class_code);
    silib_sdm_init_set_pf_type0_intr_pin(accelip_config_base_addr, p_rciep_ctxt->intr_pin);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_rcec_pf_type0(uintptr_t accelip_config_base_addr, pcie_type0_ctxt_t* p_rcec_ctxt)
{
    silib_sdm_init_set_rcec_pf_type0_ven_dev_id(accelip_config_base_addr, p_rcec_ctxt->vendor_id, p_rcec_ctxt->device_id);
    silib_sdm_init_set_rcec_pf_type0_subsystem_id(accelip_config_base_addr,
                                                  p_rcec_ctxt->ss_vendor_id,
                                                  p_rcec_ctxt->subsystem_id);
    silib_sdm_init_set_rcec_pf_type0_rev_id(accelip_config_base_addr, p_rcec_ctxt->revision_id);
    silib_sdm_init_set_rcec_pf_type0_class_code(accelip_config_base_addr,
                                                p_rcec_ctxt->class_code->prog_intf,
                                                p_rcec_ctxt->class_code->sub_class_code,
                                                p_rcec_ctxt->class_code->base_class_code);
    silib_sdm_init_set_rcec_pf_type0_intr_pin(accelip_config_base_addr, p_rcec_ctxt->intr_pin);

    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_pf_ctrl_reg(uintptr_t ext_cfg_addr, pcie_ctrl_reg_t* p_ctrl_reg)
{
    uint32_t mask;
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    _addressblock_0x100000_bcfg_boot_cfg_sdm_ctrl ctrl_reg = {{.dti_v3_protocol = p_ctrl_reg->dti_v3_protocol,
                                                               .pcie_sid_segment_id = p_ctrl_reg->pcie_sid_segment_id,
                                                               .pf_crs_resp_data = p_ctrl_reg->pf_crs_resp_data,
                                                               .vf_crs_resp_data = p_ctrl_reg->vf_crs_resp_data}};

    mask = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_SDM_CTRL_DTI_V3_PROTOCOL_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_SDM_CTRL_PCIE_SID_SEGMENT_ID_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_SDM_CTRL_PF_CRS_RESP_DATA_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_SDM_CTRL_VF_CRS_RESP_DATA_MASK;

    MMIO_UPDATE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_sdm_ctrl, ctrl_reg.as_uint32, mask);
    return SILIBS_SUCCESS;
}

static int32_t silib_sdm_init_set_pf_tid_ctrl_reg(uintptr_t ext_cfg_addr, pcie_tid_ctrl_reg_t* p_tid_ctrl)
{
    uint32_t mask;
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    _addressblock_0x100000_bcfg_boot_cfg_tid_ctrl tid_ctrl = {
        {.smmu_dti_tid = p_tid_ctrl->smmu_dti_tid, .gic_tid = p_tid_ctrl->gic_tid}};

    mask = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_TID_CTRL_SMMU_DTI_TID_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_TID_CTRL_GIC_TID_MASK;

    MMIO_UPDATE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_tid_ctrl, tid_ctrl.as_uint32, mask);

    return SILIBS_SUCCESS;
}

static int32_t sdm_init_enable_ecc(uintptr_t ext_cfg_addr, _addressblock_0x100000_bcfg_boot_cfg_ecc_dis* p_ecc_enb)
{
    uint32_t mask;
    ptr_sdm_ext_cfg_reg ext_cfg = (ptr_sdm_ext_cfg_reg)ext_cfg_addr;

    _addressblock_0x100000_bcfg_boot_cfg_ecc_dis cfg_ecc = {.lstrg_dis_ecc_chk = p_ecc_enb->lstrg_dis_ecc_chk,
                                                            .bpe_dis_ecc_chk = p_ecc_enb->bpe_dis_ecc_chk,
                                                            .atc_dis_ecc_chk = p_ecc_enb->atc_dis_ecc_chk,
                                                            .dct_dis_ecc_chk = p_ecc_enb->dct_dis_ecc_chk,
                                                            .armap_dis_parity_chk = p_ecc_enb->armap_dis_parity_chk,
                                                            .fab_dis_ecc_chk = p_ecc_enb->fab_dis_ecc_chk,
                                                            .coproc_dis_ecc_chk = p_ecc_enb->coproc_dis_ecc_chk,
                                                            .bpe_dis_parity_chk = p_ecc_enb->bpe_dis_parity_chk};

    mask = _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECC_DIS_LSTRG_DIS_ECC_CHK_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECC_DIS_BPE_DIS_ECC_CHK_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECC_DIS_ATC_DIS_ECC_CHK_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECC_DIS_DCT_DIS_ECC_CHK_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECC_DIS_ARMAP_DIS_PARITY_CHK_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECC_DIS_FAB_DIS_ECC_CHK_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECC_DIS_COPROC_DIS_ECC_CHK_MASK |
           _ADDRESSBLOCK_0X100000_BCFG_BOOT_CFG_ECC_DIS_BPE_DIS_PARITY_CHK_MASK;

    MMIO_UPDATE32(&ext_cfg->_addressblock_0x100000.bcfg_boot_cfg_ecc_dis, cfg_ecc.as_uint32, mask);

    return SILIBS_SUCCESS;
}

static void sdm_init_configure_pcie_ecam(uint64_t accelip_config_base_addr, accelip_pcie_ctxt_t* p_pcie_ctxt_t)
{
    sdm_init_set_ecam_base(accelip_config_base_addr, PCIE_ECAM_START);

    sdm_init_set_rciep_bdf(accelip_config_base_addr,
                           p_pcie_ctxt_t->p_rciep_bdf->bus,
                           p_pcie_ctxt_t->p_rciep_bdf->dev,
                           p_pcie_ctxt_t->p_rciep_bdf->func);

    sdm_init_set_rcec_bdf(accelip_config_base_addr,
                          p_pcie_ctxt_t->p_rcec_bdf->bus,
                          p_pcie_ctxt_t->p_rcec_bdf->dev,
                          p_pcie_ctxt_t->p_rcec_bdf->func);
}

static void silibs_configure_pcie_params(uint64_t accelip_config_base_addr, accelip_pcie_ctxt_t* p_pcie_ctxt_t)
{
    debug_print("PCIe Params init start\n");

    // Pre pcie config
    silib_sdm_init_set_rciep_pf_type0(accelip_config_base_addr, p_pcie_ctxt_t->p_rciep_type0_ctxt);
    silib_sdm_init_set_rcec_pf_type0(accelip_config_base_addr, p_pcie_ctxt_t->p_rcec_type0_ctxt);
    silib_sdm_init_set_pf_ctrl_reg(accelip_config_base_addr, p_pcie_ctxt_t->ctrl_reg);
    silib_sdm_init_set_pf_tid_ctrl_reg(accelip_config_base_addr, p_pcie_ctxt_t->tid_ctrl);

    // ECAM setup
    sdm_init_configure_pcie_ecam(accelip_config_base_addr, p_pcie_ctxt_t);

    // Enable ECAM
    sdm_init_enable_ecam(accelip_config_base_addr, true);

    // TODO (ADO 1728281): total VFs setting

    // TODO (ADO 1728281): total BPEs setting

    debug_print("PCIe Params init done\n");
}

static int32_t silibs_configure_emcpu_params(accelip_metadata_t* p_accelip_metadata,
                                             uint64_t accelip_config_base_addr,
                                             accelip_emcpu_ctxt_t* p_emcpu_ctxt_t)
{
    int32_t ret = SILIBS_SUCCESS;

    debug_print("Accel emCPU Params init start\n");

    // Set emCPU interrupt vector base
    sdm_init_set_emcpu_initvtor(accelip_config_base_addr, p_emcpu_ctxt_t->int_vtor);

    // Disable fence control
    sdm_init_enable_fence(accelip_config_base_addr, p_emcpu_ctxt_t->enable_fence);

    // Release from Reset
    sdm_init_deassert_nsysreset(accelip_config_base_addr);

    // Initialze SDM Memories
    sdm_init_trigger_memory_init_blocking(accelip_config_base_addr, p_emcpu_ctxt_t->p_mem_init);

    // Enable I-TCM ECC
    sdm_init_enable_itcm_ecc(accelip_config_base_addr, p_emcpu_ctxt_t->enable_itcm_ecc);

    // Enable D-TCM ECC
    sdm_init_enable_dtcm_ecc(accelip_config_base_addr, p_emcpu_ctxt_t->enable_dtcm_ecc);

    // Enable SDM ECC
    // TODO: ADO 1875811: Enable CDED ECC
    if (p_accelip_metadata->accel_type == ACCELERATOR_SDMSS)
    {
        sdm_init_enable_ecc(accelip_config_base_addr, p_emcpu_ctxt_t->p_cfg_ecc);
    }

    // Check for emCPU Firmware Download Readiness
    // TODO (ADO 1728282): Not supported in SVP so commented.
    // ret = sdm_init_wait_until_ahbs_ready(accelip_config_base_addr);

    // Boot emCPU Firmware
    sdm_init_disable_cpu_wait(accelip_config_base_addr);

    debug_print("Accel emCPU Params init done\n");

    return ret;
}

static int32_t configure_accel_ip(accelip_metadata_t* p_accelip_metadata,
                                  accelip_ctxt_t* p_accelip_ctxt,
                                  atu_mapping_ctxt_t* p_accelss_atu_mapping_ctxt,
                                  sdm_pre_pcie_cfg_t* p_pre_pcie_cfg)
{
    atu_map_entry_t* p_atu_map_entry = p_accelss_atu_mapping_ctxt->p_atu_map_entry;
    atu_id_t atu_id = p_accelss_atu_mapping_ctxt->ss_atu_id;

    debug_print("accel lib: Configure Accel IP Start\n");

    // Create ATU Mapping (SCP View) for the Accel IP
    int32_t ret = atu_map(atu_id, p_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Accel IP ATU Mapping failed\n");
        ret = ACCEL_RET_FAIL_ATU_MAP;
        /*
         * TODO: ADO 1889561: Remove this goto and rather return from here as
         * there is nothing to unwind here.
         */
        goto exit;
    }

    // Get Accel subsystem mscp start and Accel IP base addr after ATU mapped successfully
    uint64_t mscp_start_address = p_atu_map_entry->mscp_start_address;
    uint64_t accel_ip_ext_config_base_addr = mscp_start_address;

    // TODO: ADO 1831262: Make the init flow for `ACCELERATOR_SDMSS` and
    // `ACCELERATOR_CDEDSS` same.
    if (p_accelip_metadata->accel_type == ACCELERATOR_SDMSS)
    {
        accel_ip_ext_config_base_addr += SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS;

        // Configure PCIe params
        silibs_configure_pcie_params(accel_ip_ext_config_base_addr, p_accelip_ctxt->p_pcie_ctxt);
    }
    else if (p_accelip_metadata->accel_type == ACCELERATOR_CDEDSS)
    {
        accel_ip_ext_config_base_addr += CDEDSS_CONFIG_SDM_EXT_CFG_ADDRESS;

        // Initialize Pre-PCIe Config
        sdm_init_write_pre_pcie_cfg(accel_ip_ext_config_base_addr, p_pre_pcie_cfg);

        // Configure PCIe eCAM
        sdm_init_configure_pcie_ecam(accel_ip_ext_config_base_addr, p_accelip_ctxt->p_pcie_ctxt);

        // Enable PCIe eCAM
        sdm_init_enable_ecam(accel_ip_ext_config_base_addr, true);
    }

    // Configure emCPU (M7) params
    silibs_configure_emcpu_params(p_accelip_metadata, accel_ip_ext_config_base_addr, p_accelip_ctxt->p_emcpu_ctxt);

    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        // Configure DTI/LTI params
        silibs_configure_smmu_connection(accel_ip_ext_config_base_addr);
    }

    // Destroy ATU mapping
    ret = atu_unmap(atu_id, p_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Accel IP ATU Unmapping failed\n");
        /*
         * TODO: ADO 1889561: Return from here as there is nothing to unwind here.
         */
        ret = ACCEL_RET_FAIL_ATU_UNMAP;
    }

exit:
    debug_print("accel lib: Configure Accel IP Done\n");

    return ret;
}

#if defined(FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP)
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
#endif /* FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP */

static int32_t init_accelerator(subsystem_ctxt_t* p_ss_ctxt)
{
    int32_t ret = ACCEL_RET_SUCCESS;

    if (p_ss_ctxt->accelip_metadata.accel_type == ACCELERATOR_CDEDSS)
    {
#ifdef FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP
        // TODO: WA until HSP configures CDEDSS Tower
        atu_map_entry_t cdeedss_tower_atu_map_entry = {0};

        if (idsw_get_platform_sdv() == PLATFORM_SVP_SIM)
        {
            ret = init_hsp_cdedss_tower(&cdeedss_tower_atu_map_entry);
            if (ret != ACCEL_RET_SUCCESS)
            {
                debug_print("accel_lib: WA: CDEDSS Tower Init failed\n");
                return ret;
            }
        }
        else
#endif /* FEATURE_SVP_WA_INIT_CDEDSS_TOWER_ON_BEHALF_OF_HSP */
        {
            /*
             * Since HSP FW is not ready for the handshake, we would be consuming
             * the cmm scripts from FPGA team to configure the CDEDSS Tower.
             *
             * Once HSP FW is available, we need to follow the below steps:
             *   - Send request to HSP to configure the CDEDSS Tower via Mailbox.
             *   - Wait for the confirmation from HSP via Mailbox about the status
             *     to configure the CDEDSS Tower.
             *   - Take further action based on SUCCESS/FAIL.
             *
             * This ADO catpures the status of HSP FW readiness: 1568266
             */
        }
    }

    // Configure Accelerator SubSystem Tower
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        ret = configure_accel_subsystem_pcr(p_ss_ctxt->accelip_metadata,
                                            p_ss_ctxt->p_accelss_tower_attr,
                                            p_ss_ctxt->p_accelss_atu_mapping_ctxt);
        if (ret != ACCEL_RET_SUCCESS)
        {
            critical_print("Accel Subsystem PCR configuration failed.\n");
            return ACCEL_RET_FAIL_SS_PCR;
        }
    }

    // Configure AccelIP pre boot registers
    ret = configure_accel_ip(&(p_ss_ctxt->accelip_metadata),
                             p_ss_ctxt->p_accelip_ctxt,
                             p_ss_ctxt->p_accelss_atu_mapping_ctxt,
                             p_ss_ctxt->p_pre_pcie_cfg);
    if (ret != ACCEL_RET_SUCCESS)
    {
        critical_print("Accel IP configuration failed.\n");
        ret = ACCEL_RET_FAIL_ACCEL_IP;
        goto exit;
    }

exit:
    return ret;
}

/*----------------------------- Global Functions ----------------------------*/
int32_t scp_accelerators_init(void)
{
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    int ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    FPFW_RUNTIME_ASSERT(p_ss_ctxt != NULL);

    printf("Number of Accelerator intances present: %d\n", (int)accel_ctxt_size);

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO (ADO 1728772) : init any particular accelerator instance only if that is enabled in fuse
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            printf("accel lib: Initializing for die_id=%d, accel_type=%d, accel_instance=%d\n",
                   p_ss_ctxt[index].accelip_metadata.die_instance,
                   p_ss_ctxt[index].accelip_metadata.accel_type,
                   p_ss_ctxt[index].accelip_metadata.accel_instance);

            ret = init_accelerator(&p_ss_ctxt[index]);

            /*
             * TODO: ADO 1889561: If we are asserting here, then there's really
             * no point of a return from the function.
             */
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);
        }
    }

    return ret;
}
