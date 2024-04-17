//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_configs.c
 * This file provides the configurations needed to initialize the Accelerator
 * IP(s) available on the SoC.
 */

/*-------------------------------- Includes ---------------------------------*/
#include "silibs_ap_top_regs.h"
#include "silibs_kng_soc.h"
#include "vab_sdm_top_regs.h"

#include <accelerator_ip_priv.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

/* -- Die specific Registers/Offsets/Configs -- */
/* Die0 - VAB IP for SDMSS */
#define D0_SDMSS_VAB_ADDR                  (AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_VAB_ADDRESS)
#define D0_SDMSS_VAB_TOWER_ADDRESS         (AP_TOP_D0_VAB_SDM_ADDRESS + VAB_VAB_TOWER_ADDRESS)
#define D0_SDMSS_VAB_TOWER_SAM_ASNI_OFFSET (VAB_ASNI_SAM_OFFSET) // Defined in Silibs tower_vab.h

/* Die1 - VAB IP for SDMSS */
#define D1_SDMSS_VAB_ADDR (AP_TOP_D1_VAB_SDM_ADDRESS + VAB_SDM_TOP_VAB_ADDRESS)

/* Die0 - VAB IP for CDEDSS */
#define D0_CDEDSS_VAB_ADDR (AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_VAB_ADDRESS)
#define D0_CDEDSS_VAB_TOWER_ADDRESS \
    (AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_VAB_TOWER_ADDRESS) // FIXME: Confirm if the VAB_VAB_TOWER_ADDRESS holds true for CDED as well
#define D0_CDEDSS_VAB_TOWER_SAM_ASNI_OFFSET (VAB_ASNI_SAM_OFFSET) // Defined in Silibs tower_vab.h

/* Die1 - VAB IP for CDEDSS */
#define D1_CDEDSS_VAB_ADDR (AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_VAB_ADDRESS)

/* Die0 - SDMSS IP Related */
#define D0_SDMSS_BASE_ADDR       (AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS)
#define D0_SDMSS_CONFIG_ADDR     (AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS)
#define D0_SDMSS_TOWER_BASE_ADDR (AP_TOP_D0_VAB_SDM_ADDRESS + SDMSS_CONFIG_SDMSS_TOWER_ADDRESS)
#define D0_SDMSS_TOWER_SAM_ASNI_OFFSET \
    (NOCNI_NITOWER_SDM_2_U_ASNI_VAB_AXI_ASNI_CORE_SDMSS_SAM_UNIT_INFO_ADDRESS)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/
static atu_map_entry_t die0_sdmss_instance0_vab_atu_map_entry = {
    // D0-SDMSS0
    .ap_base_address = AP_TOP_D0_VAB_SDM_ADDRESS,
    .mscp_start_address = 0,
    .mscp_end_address = AP_TOP_D0_VAB_SDM_SIZE - 1,
    .attribute = {.axprot0 = 0x3, .axprot1 = 0x2, .axnse = 0x3},
};

static atu_map_entry_t die0_sdmss_instance0_accelss_atu_map_entry = {
    // D0-SDMSS0
    .ap_base_address = AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS,
    .mscp_start_address = 0,
    .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_SDM_SIZE, ATU_PAGE_SIZE) - 1,
    .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
};

static atu_mapping_ctxt_t die0_sdmss_instance0_vab_atu_mapping_ctxt = {ATU_ID_MSCP, &die0_sdmss_instance0_vab_atu_map_entry};
static atu_mapping_ctxt_t die0_sdmss_instance0_accelss_atu_mapping_ctxt = {ATU_ID_MSCP, &die0_sdmss_instance0_accelss_atu_map_entry};

static tower_attr_t die0_sdmss_instance0_vab_tower_attr = {0}; // Will be updated at run time after ATU mapping done

static tower_attr_t die0_sdmss_instance0_accelss_tower_attr = {0}; // Will be updated at run time after ATU mapping done

static pcie_ctrl_reg_t die0_sdmss_instance0_ctrl_reg_attr = {
    .dti_v3_protocol = BCFG_BOOT_CFG_SDM_CTRL_DTI_V3_PROTOCOL, // (DTI ATS Protocol v3)
    .pcie_sid_segment_id = BCFG_BOOT_CFG_SDM_CTRL_PCIE_SID_SEGMENT_ID,
    .pf_crs_resp_data = BCFG_BOOT_CFG_SDM_CTRL_PF_CRS_RESP,
    .vf_crs_resp_data = BCFG_BOOT_CFG_SDM_CTRL_VF_CRS_RESP_DATA};

static pcie_tid_ctrl_reg_t die0_sdmss_instance0_tid_ctrl_reg_attr = {.smmu_dti_tid = BCFG_BOOT_CFG_TID_CTRL_SMMU_DTI_TID,
                                                                     .gic_tid = BCFG_BOOT_CFG_TID_CTRL_GIC_TID};

static pcie_bdf_t die0_sdmss_instance0_rciep_bdf_attr = {.bus = D0_SDM_RCIEP_BUS, .dev = 0, .func = 0};

static pcie_bdf_t die0_sdmss_instance0_rcec_bdf_attr = {.bus = D0_SDM_RCEC_BUS, .dev = 0, .func = 0};

static pcie_class_code_t die0_sdmss_instance0_rciep_class_code_attr = {
    .prog_intf = BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_PROG_INTF,
    .sub_class_code = BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_SUB_CLASS_CODE,
    .base_class_code = BCFG_BOOT_CFG_PF_TYPE0_CLASS_CODE_BASE_CLASS_CODE};

static pcie_class_code_t die0_sdmss_instance0_rcec_class_code_attr = {
    .prog_intf = BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_PROG_INTF,
    .sub_class_code = BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_SUB_CLASS_CODE,
    .base_class_code = BCFG_BOOT_CFG_RCEC_TYPE0_CLASS_CODE_BASE_CLASS_CODE};

static pcie_type0_ctxt_t die0_sdmss_instance0_rciep_type0_attr = {
    BCFG_BOOT_CFG_PF_TYPE0_SDM_ID_VEN_ID,          // vendor id
    BCFG_BOOT_CFG_PF_TYPE0_SDM_ID_DEV_ID,          // device id
    BCFG_BOOT_CFG_PF_TYPE0_SS_ID_SUBSYS_VENDOR_ID, // ss_vendor_id
    BCFG_BOOT_CFG_PF_TYPE0_SS_ID_SUBSYS_ID,        // sybsystem id
    BCFG_BOOT_CFG_PF_TYPE0_REV_REV_ID,             // revision id
    &die0_sdmss_instance0_rciep_class_code_attr,   // class code
    BCFG_BOOT_CFG_PF_TYPE0_INTR_PIN};              // interrupt pin INTA

static pcie_type0_ctxt_t die0_sdmss_instance0_rcec_type0_attr = {
    BCFG_BOOT_CFG_RCEC_TYPE0_SDM_ID_VEN_ID,          // vendor id
    BCFG_BOOT_CFG_RCEC_TYPE0_SDM_ID_DEV_ID,          // device id
    BCFG_BOOT_CFG_RCEC_TYPE0_SS_ID_SUBSYS_VENDOR_ID, // ss_vendor_id
    BCFG_BOOT_CFG_RCEC_TYPE0_SS_ID_SUBSYS_ID,        // sybsystem id
    BCFG_BOOT_CFG_RCEC_TYPE0_REV_REV_ID,             // revision id
    &die0_sdmss_instance0_rcec_class_code_attr,      // class code
    BCFG_BOOT_CFG_RCEC_TYPE0_INTR_PIN};              // interrupt pin INTB

// Setting whole struct to 0 == false
static sdm_mem_init_t die0_sdmss_instance0_mem_init_attr = {0};

// Setting whole struct to 0 == false
static _addressblock_0x100000_bcfg_boot_cfg_ecc_dis die0_sdmss_instance0_ecc_dis = {0};

static accelip_pcie_ctxt_t die0_sdmss_instance0_pcie_ctxt = {&die0_sdmss_instance0_rciep_type0_attr,
                                                             &die0_sdmss_instance0_rcec_type0_attr,
                                                             &die0_sdmss_instance0_ctrl_reg_attr,
                                                             &die0_sdmss_instance0_tid_ctrl_reg_attr,
                                                             &die0_sdmss_instance0_rciep_bdf_attr, // bdf details
                                                             &die0_sdmss_instance0_rcec_bdf_attr}; // bdf details

static accelip_emcpu_ctxt_t die0_sdmss_instance0_emcpu_ctxt = {&die0_sdmss_instance0_mem_init_attr,
                                                               DIE0_SDMSS_INSTANCE0_INT_VECTOR,
                                                               false, // Enable fence
                                                               true,  // Enable I-TCM
                                                               true,  // Enable D-TCM
                                                               &die0_sdmss_instance0_ecc_dis};

static vab_ctxt_t die0_sdmss_instance0_vab_ctxt = {&die0_sdmss_instance0_vab_tower_attr};

static accelip_ctxt_t die0_sdmss_instance0_accelip_ctxt = {&die0_sdmss_instance0_pcie_ctxt, &die0_sdmss_instance0_emcpu_ctxt};

static subsystem_ctxt_t ss_ctxts[] = {
    // D0 SDMSS Instance 0
    {
        {.die_instance = SOC_D0, .accel_type = ACCELERATOR_SDMSS, .accel_instance = 0},
        &die0_sdmss_instance0_vab_ctxt,
        &die0_sdmss_instance0_vab_atu_mapping_ctxt,
        &die0_sdmss_instance0_accelss_tower_attr,
        &die0_sdmss_instance0_accelss_atu_mapping_ctxt,
        &die0_sdmss_instance0_accelip_ctxt // accel details
    },

    // TBD
    // D0 CDEDSS Instance 0
    // D1 SDMSS Instance 0
    // D1 CDEDSS Instance 0
};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
subsystem_ctxt_t* get_accelerator_ctxt(uint32_t* accel_instance_size)
{
    *accel_instance_size = ARRAY_SIZE(ss_ctxts);
    return ss_ctxts;
}
