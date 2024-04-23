//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip.c
 * This file provides interface to initializes the Accelerator IP(s) available
 * on the SoC.
 */

/*-------------------------------- Includes ---------------------------------*/
#include "vab_sdm_top_regs.h"

#include <FpFwAssert.h>
#include <accelerator_ip_priv.h>
#include <debug.h>
#include <idsw.h>
#include <pcr_rpss.h>
#include <silibs_ap_top_regs.h>
#include <silibs_kng_soc.h>
#include <smmu.h>
#include <tower_sdmss.h>
#include <vab_pcr_init.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

#define ACCEL_PCR_CONFIG_TIMEOUT (10)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/
static int32_t configure_vab(vab_ctxt_t* p_vab_ctxt, atu_mapping_ctxt_t* p_vab_atu_mapping_ctxt)
{
    atu_map_entry_t* p_atu_map_entry = p_vab_atu_mapping_ctxt->p_atu_map_entry;
    atu_id_t atu_id = p_vab_atu_mapping_ctxt->ss_atu_id;
    int32_t sts = SILIBS_SUCCESS;

    // TODO (ADO 1728276): fix this after silib VAB code refactoring is done
    UNUSED(p_vab_ctxt);

    // Map ATU
    int32_t ret = atu_map(atu_id, p_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Vab ATU Mapping failed");
        ret = ACCEL_RET_FAIL_ATU_MAP;
        goto exit;
    }

    // Get vab base address and tower base address after ATU mapped successfully
    uint64_t vab_base_addr = p_atu_map_entry->ap_base_address;
    uint64_t mscp_start_address = p_atu_map_entry->mscp_start_address;

    // Configure VAB SAM
    configure_vab_system_addr_map(vab_base_addr, mscp_start_address + VAB_VAB_TOWER_ADDRESS);
    debug_print("Vab SAM Configuration Completed");

    // TODO (ADO 1728276): PCR is not implemented in SVP
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        // Configure VAB PCR
        deassert_pcr_reset(mscp_start_address + VAB_VAB_PCR_TOP_ADDRESS);
        ret = vab_pcr_init(mscp_start_address + VAB_VAB_PCR_TOP_ADDRESS);
        if (ret != SILIBS_SUCCESS)
        {
            debug_print("Vab PCR configuration failed");
            ret = ACCEL_RET_FAIL_VAB_PCR;
            goto exit_atu_unmap;
        }
    }

    // Configure SMMU. Current configuration is in bypass mode only
    uint64_t scp_smmu_base_addr = mscp_start_address + VAB_SMMU_YARDLEY_VAB_ADDRESS;
    smmu_enable_access(scp_smmu_base_addr);
    ret = smmu_enable_access_check(scp_smmu_base_addr);

    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Vab SMMU is not enabled");
        ret = ACCEL_RET_FAIL_SMMU_ENABLE;
        goto exit_atu_unmap;
    }

exit_atu_unmap:
    // Destroy ATU Mapping (SCP View) for the given VAB instance
    sts = atu_unmap(atu_id, p_atu_map_entry);
    if (sts != SILIBS_SUCCESS)
    {
        debug_print("Vab ATU Unmapping failed");
    }

exit:
    return ((ret != SILIBS_SUCCESS) ? ret : sts);
}

static PCR_RPSS_INSTANCE get_ss_pcr_type(eACCELERATOR_TYPE accel_type)
{
    PCR_RPSS_INSTANCE ss_pcr_type;

    switch (accel_type)
    {
    case ACCELERATOR_SDMSS:
        ss_pcr_type = PCR_SDMSS0;
        break;
    case ACCELERATOR_CDEDSS:
        ss_pcr_type = PCR_CDEDSS0;
        break;
    default:
        ss_pcr_type = NUM_PCR_RPSS_INSTANCES;
    }

    return ss_pcr_type;
}

static int32_t accelss_init_pcr(eACCELERATOR_TYPE accel_type, uint32_t accelss_pcr_base_addr)
{
    int32_t ret = ACCEL_RET_SUCCESS;

    PCR_RPSS_INSTANCE ss_pcr_type = get_ss_pcr_type(accel_type);
    if (ss_pcr_type == NUM_PCR_RPSS_INSTANCES)
    {
        return ACCEL_RET_FAIL_SS_PCR;
    }

    pcr_rpss_entity_t* pcr = pcr_rpss_get_entity(ss_pcr_type);
    pcr_rpss_set_base(pcr, accelss_pcr_base_addr);
    if (pcr_rpss_verify(pcr) != SILIBS_SUCCESS)
    {
        return ACCEL_RET_FAIL_SS_PCR;
    }

    pcr_rpss_configure_clocks(pcr, ACCEL_PCR_CONFIG_TIMEOUT);
    pcr_rpss_deassert_por_reset(pcr);

    return ret;
}

static int32_t configure_accel_subsystem_tower(accelip_metadata_t accelip_metadata,
                                               tower_attr_t* p_accelss_tower_attr,
                                               atu_mapping_ctxt_t* p_accelss_atu_mapping_ctxt)
{
    atu_map_entry_t* p_atu_map_entry = p_accelss_atu_mapping_ctxt->p_atu_map_entry;
    atu_id_t atu_id = p_accelss_atu_mapping_ctxt->ss_atu_id;
    int32_t sts = SILIBS_SUCCESS;

    // Will be used when more IP of this tower will be configured.
    UNUSED(p_accelss_tower_attr);

    debug_print("Accel SS init start");

    // Create ATU Mapping (SCP View) for the Accel SS
    int32_t ret = atu_map(atu_id, p_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Accel SS ATU Mapping failed");
        return ACCEL_RET_FAIL_ATU_MAP;
    }

    // Get Accel subsystem base address and tower base address after ATU mapped successfully
    uint32_t mscp_start_address = p_atu_map_entry->mscp_start_address;

    if (accelip_metadata.accel_type == ACCELERATOR_SDMSS)
    {
        configure_sdmss_system_addr_map(mscp_start_address + SDMSS_CONFIG_SDMSS_TOWER_ADDRESS,
                                        (SDMSS_INSTANCE)accelip_metadata.die_instance);
    }

    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        // Configure Accel SS PCR
        ret = accelss_init_pcr(accelip_metadata.accel_type, mscp_start_address + SDMSS_CONFIG_SDMSS_PCR_TOP_ADDRESS);
        if (ret != SILIBS_SUCCESS)
        {
            debug_print("Accel SS PCR configuration failed");
            ret = ACCEL_RET_FAIL_SS_PCR;
            goto exit_atu_unmap;
        }
    }

    debug_print("Accel SS init done");

exit_atu_unmap:
    // Destroy ATU Mapping (SCP View) for the given VAB instance
    sts = atu_unmap(atu_id, p_atu_map_entry);
    if (sts != SILIBS_SUCCESS)
    {
        debug_print("Accel SS ATU Unmapping failed");
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

static void silibs_configure_pcie_params(uint64_t accelip_config_base_addr, accelip_pcie_ctxt_t* p_pcie_ctxt_t)
{
    debug_print("PCIe Params init start");

    // Pre pcie config
    silib_sdm_init_set_rciep_pf_type0(accelip_config_base_addr, p_pcie_ctxt_t->p_rciep_type0_ctxt);
    silib_sdm_init_set_rcec_pf_type0(accelip_config_base_addr, p_pcie_ctxt_t->p_rcec_type0_ctxt);
    silib_sdm_init_set_pf_ctrl_reg(accelip_config_base_addr, p_pcie_ctxt_t->ctrl_reg);
    silib_sdm_init_set_pf_tid_ctrl_reg(accelip_config_base_addr, p_pcie_ctxt_t->tid_ctrl);

    // ECAM setup
    sdm_init_set_ecam_base(accelip_config_base_addr, PCIE_ECAM_START);
    sdm_init_set_rciep_bdf(accelip_config_base_addr,
                           p_pcie_ctxt_t->p_rciep_bdf->bus,
                           p_pcie_ctxt_t->p_rciep_bdf->dev,
                           p_pcie_ctxt_t->p_rciep_bdf->func);
    sdm_init_set_rcec_bdf(accelip_config_base_addr,
                          p_pcie_ctxt_t->p_rcec_bdf->bus,
                          p_pcie_ctxt_t->p_rcec_bdf->dev,
                          p_pcie_ctxt_t->p_rcec_bdf->func);

    // Enable ECAM
    sdm_init_enable_ecam(accelip_config_base_addr, true);

    // TODO (ADO 1728281): total VFs setting

    // TODO (ADO 1728281): total BPEs setting

    debug_print("PCIe Params init done");
}

static int32_t silibs_configure_emcpu_params(uint64_t accelip_config_base_addr, accelip_emcpu_ctxt_t* p_emcpu_ctxt_t)
{
    int32_t ret = SILIBS_SUCCESS;

    debug_print("Accel emCPU Params init start");

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
    sdm_init_enable_ecc(accelip_config_base_addr, p_emcpu_ctxt_t->p_cfg_ecc);

    // Check for emCPU Firmware Download Readiness
    // TODO (ADO 1728282): Not supported in SVP so commented.
    // ret = sdm_init_wait_until_ahbs_ready(accelip_config_base_addr);

    // Boot emCPU Firmware
    sdm_init_disable_cpu_wait(accelip_config_base_addr);

    debug_print("Accel emCPU Params init done");

    return ret;
}

static int32_t configure_accel_ip(accelip_ctxt_t* p_accelip_ctxt, atu_mapping_ctxt_t* p_accelss_atu_mapping_ctxt)
{
    atu_map_entry_t* p_atu_map_entry = p_accelss_atu_mapping_ctxt->p_atu_map_entry;
    atu_id_t atu_id = p_accelss_atu_mapping_ctxt->ss_atu_id;

    // Create ATU Mapping (SCP View) for the Accel IP
    int32_t ret = atu_map(atu_id, p_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Accel IP ATU Mapping failed");
        ret = ACCEL_RET_FAIL_ATU_MAP;
        goto exit;
    }

    // Get Accel subsystem mscp start and Accel IP base addr after ATU mapped successfully
    uint64_t mscp_start_address = p_atu_map_entry->mscp_start_address;
    uint64_t accel_ip_config_base_addr = mscp_start_address + SDMSS_CONFIG_SDM_EXT_CFG_ADDRESS;

    // Configure PCIe params
    silibs_configure_pcie_params(accel_ip_config_base_addr, p_accelip_ctxt->p_pcie_ctxt);

    // Configure emCPU (M7) params
    silibs_configure_emcpu_params(accel_ip_config_base_addr, p_accelip_ctxt->p_emcpu_ctxt);

    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        // Configure DTI/LTI params
        silibs_configure_smmu_connection(accel_ip_config_base_addr);
    }

    // Destroy ATU mapping
    ret = atu_unmap(atu_id, p_atu_map_entry);
    if (ret != SILIBS_SUCCESS)
    {
        debug_print("Accel IP ATU Unmapping failed");
        ret = ACCEL_RET_FAIL_ATU_UNMAP;
    }

exit:
    return ret;
}

static int32_t init_accelerator(subsystem_ctxt_t* p_ss_ctxt)
{
    // Configure VAB (SAM, APU, FMU, PMU)
    int32_t ret = configure_vab(p_ss_ctxt->p_vab_ctxt, p_ss_ctxt->p_vab_atu_mapping_ctxt);
    if (ret != ACCEL_RET_SUCCESS)
    {
        critical_print("VAB configuration failed.");
        return ACCEL_RET_FAIL_VAB;
    }

    // Configure Accel SubSystem tower
    ret = configure_accel_subsystem_tower(p_ss_ctxt->accelip_metadata,
                                          p_ss_ctxt->p_accelss_tower_attr,
                                          p_ss_ctxt->p_accelss_atu_mapping_ctxt);
    if (ret != ACCEL_RET_SUCCESS)
    {
        critical_print("Accel Subsystem configuration failed.");
        return ACCEL_RET_FAIL_TOWER;
    }

    // Configure AccelIP pre boot registers
    ret = configure_accel_ip(p_ss_ctxt->p_accelip_ctxt, p_ss_ctxt->p_accelss_atu_mapping_ctxt);
    if (ret != ACCEL_RET_SUCCESS)
    {
        critical_print("Accel IP configuration failed.");
        return ACCEL_RET_FAIL_ACCEL_IP;
    }

    return ACCEL_RET_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/
int32_t scp_accelerators_init(void)
{
    DIE_INSTANCE current_die_instance = (DIE_INSTANCE)idsw_get_die_id();
    int ret = ACCEL_RET_SUCCESS;
    uint32_t accel_ctxt_size = 0;

    subsystem_ctxt_t* p_ss_ctxt = get_accelerator_ctxt(&accel_ctxt_size);

    FPFW_RUNTIME_ASSERT(p_ss_ctxt != NULL);

    printf("Number of Accelerator intances present : %d\n", (int)accel_ctxt_size);

    // Init all available Accelerator instances
    for (uint32_t index = 0; index < accel_ctxt_size; index++)
    {
        // TODO (ADO 1728772) : init any particular accelerator instance only if that is enabled in fuse
        if (p_ss_ctxt[index].accelip_metadata.die_instance == current_die_instance)
        {
            ret = init_accelerator(&p_ss_ctxt[index]);
            FPFW_RUNTIME_ASSERT(ret == ACCEL_RET_SUCCESS);
        }
    }

    return ret;
}
