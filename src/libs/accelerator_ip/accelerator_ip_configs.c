//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_configs.c
 * This file provides the configurations needed to initialize the Accelerator
 * IP(s) available on the SoC.
 */

/*-------------------------------- Includes ---------------------------------*/
#include "_addressblock_0x100000_regs.h" // for _addressblock_0x100000_bcfg...
#include "accelerator_ip.h"              // for ACCELERATOR_SDMSS, atu_mapp...
#include "accelerator_ip_bcfg_params.h"  // for BCFG_BOOT_CFG_PF_TYPE0_CLAS...
#include "accelerator_ip_pcie_params.h"  // for pcie_bdf_t, pcie_class_code_t
#include "atu_lib.h"                     // for ATU_ID_MSCP, atu_map_entry_t
#include "kng_soc_constants.h"           // for SOC_D0, ATU_PAGE_SIZE
#include "nocni_nitower_sdm_2_regs.h"    // for NOCNI_NITOWER_SDM_2_U_ASNI_...
#include "sdm_init.h"                    // for sdm_mem_init_t
#include "sdmss_config_regs.h"           // for SDMSS_CONFIG_SDMSS_TOWER_AD...
#include "silibs_common.h"               // for ALIGN_UP, ARRAY_SIZE
#include "silibs_kng_soc.h"              // for D0_SDM_RCEC_BUS, D0_SDM_RCI...
#include "vab_regs.h"                    // for VAB_VAB_TOWER_ADDRESS

#include <accelerator_ip_priv.h>    // for AP_TOP_D0_VAB_SDM_ADDRESS
#include <sdm_init_knobs.h>         // for MSFT_VENDOR_ID, CDED_PCI_DE...
#include <stdbool.h>                // for true, false
#include <stddef.h>                 // for NULL
#include <stdint.h>                 // for uint32_t
#include <vab_cded_ioss_top_regs.h> // for VAB_CDED_IOSS_TOP_VAB_ADDRESS

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
    .ap_base_address = AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_VAB_ADDRESS,
    .mscp_start_address = 0,
    .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_SDM_SIZE, ATU_PAGE_SIZE) - 1,
    .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
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

// TODO : Need to restore original value https://azurecsi.visualstudio.com/Dev/_workitems/edit/1805683
static sdm_mem_init_t die0_sdmss_instance0_mem_init_attr = {{true, true, true, true, true, false}};

// TODO : Need to restore original value https://azurecsi.visualstudio.com/Dev/_workitems/edit/1805683
static _addressblock_0x100000_bcfg_boot_cfg_ecc_dis die0_sdmss_instance0_ecc_dis = {
    {true, true, true, true, false, true, true, true, true, false}};

static accelip_pcie_ctxt_t die0_sdmss_instance0_pcie_ctxt = {&die0_sdmss_instance0_rciep_type0_attr,
                                                             &die0_sdmss_instance0_rcec_type0_attr,
                                                             &die0_sdmss_instance0_ctrl_reg_attr,
                                                             &die0_sdmss_instance0_tid_ctrl_reg_attr,
                                                             &die0_sdmss_instance0_rciep_bdf_attr, // bdf details
                                                             &die0_sdmss_instance0_rcec_bdf_attr}; // bdf details

static accelip_emcpu_ctxt_t die0_sdmss_instance0_emcpu_ctxt = {&die0_sdmss_instance0_mem_init_attr,
                                                               DIE0_SDMSS_INSTANCE0_INT_VECTOR,
                                                               false, // Enable fence
                                                               false, // Disable I-TCM ECC for FPGA, need to restore, WI 1805683
                                                               false, // Disable D-TCM ECC for FPGA, need to restore, WI 1805683
                                                               &die0_sdmss_instance0_ecc_dis};

static vab_ctxt_t die0_sdmss_instance0_vab_ctxt = {&die0_sdmss_instance0_vab_tower_attr};

static accelip_ctxt_t die0_sdmss_instance0_accelip_ctxt = {&die0_sdmss_instance0_pcie_ctxt, &die0_sdmss_instance0_emcpu_ctxt};

/* ---- D0 CDEDSS Instance 0 (D0-CDEDSS0) Context data-structures ----------- */

// ---- ATU Map for VAB5 ---- //
static atu_map_entry_t die0_cdedss_instance0_vab_atu_map_entry = {
    .ap_base_address = AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_VAB_ADDRESS,
    .mscp_start_address = 0,
    .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_CDED_IOSS_SIZE, ATU_PAGE_SIZE) - 1,
    .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
};

// ---- VAB Context - ATU Map for VAB5 ---- //
static atu_mapping_ctxt_t die0_cdedss_instance0_vab_atu_mapping_ctxt = {ATU_ID_MSCP, &die0_cdedss_instance0_vab_atu_map_entry};

// ---- VAB Tower ---- //
static tower_attr_t die0_cdedss_instance0_vab_tower_attr = {0}; // Will be updated at run time after ATU mapping done

// ---- VAB Context ---- //
static vab_ctxt_t die0_cdedss_instance0_vab_ctxt = {&die0_cdedss_instance0_vab_tower_attr};

// ---- ATU Map for Accel i.e. CDEDSS0 ---- //
static atu_map_entry_t die0_cdedss_instance0_accelss_atu_map_entry = {
    .ap_base_address = AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_CDEDSS_ADDRESS,
    .mscp_start_address = 0,
    .mscp_end_address = ALIGN_UP(AP_TOP_D0_VAB_CDED_IOSS_SIZE, ATU_PAGE_SIZE) - 1,
    .attribute = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT},
};

// ---- AccelSS ATU Map Context ---- //
static atu_mapping_ctxt_t die0_cdedss_instance0_accelss_atu_mapping_ctxt = {ATU_ID_MSCP, &die0_cdedss_instance0_accelss_atu_map_entry};

// ---- AccelSS - Silibs Pre-PCIe Config ---- //
static sdm_pre_pcie_cfg_t die0_cdedss_instance0_pre_pcie_cfg = {
    // PF RCiEP PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                        // Revision ID of the device
        .pci_t0_base_class_code = CDED_BASE_CLASS_CODE, // Base class code of the device
        .pci_t0_sub_class_code = CDED_SUB_CLASS_CODE,   // Sub class code of the device
        .pci_t0_subsystem_id = CDED_PCI_DEVICE_ID,      // Subsystem ID of the device
        .pci_t0_subsystem_vendor_id = MSFT_VENDOR_ID,   // Subsystem vendor ID of the device
        .pci_t0_device_id = CDED_PCI_DEVICE_ID,         // PCI device ID
        .pci_t0_vendor_id = MSFT_VENDOR_ID,             // PCI vendor ID
        .pci_t0_int_pin = INT_PIN_A,                    // Legacy interrupt pin setting
    },

    // VF RCiEP PCI T0 Cfg
    {0},

    // RCEC PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                        // Revision ID of the device
        .pci_t0_base_class_code = RCEC_BASE_CLASS_CODE, // Base class code of the device
        .pci_t0_sub_class_code = RCEC_SUB_CLASS_CODE,   // Sub class code of the device
        .pci_t0_subsystem_id = RCEC_PCI_DEVICE_ID,      // Subsystem ID of the device
        .pci_t0_subsystem_vendor_id = MSFT_VENDOR_ID,   // Subsystem vendor ID of the device
        .pci_t0_device_id = RCEC_PCI_DEVICE_ID,         // PCI device ID
        .pci_t0_vendor_id = MSFT_VENDOR_ID,             // PCI vendor ID
        .pci_t0_int_pin = INT_PIN_B,                    // Legacy interrupt pin setting
    },

    .dti_v3_protocol = DTI_ATS_V3, // DTI protocol to use (v2 or v3)

    // Response to config access timeout
    .pf_cfg_timeout_response = CRS_RESPONSE,
    .vf_cfg_timeout_response = CRS_RESPONSE,

    .tee_io_supp = true, // TODO: ADO 1885063: Enable TEE IO support capability

    .gic_tid = 0,      // GIC TID
    .smmu_dti_tid = 0, // SMMU DTI TID

    .gpa_pasid_en = false,     // Enable GPA PASID support for Rd/Wr address pointers
    .cpl_gpa_pasid_en = false, // Enable GPA PASID support for completion address pointers

    .tdisp_allow_t_cfg_wrt = false, // Allow Protected registers to be updated
                                    // while in the TDISP RUN or CONFIG_LOCKED
                                    // state and the register write has the T
                                    // bit set
    .tdisp_allow_t_cmd_sub = false, // Allow T bit enabled Command Submissions
                                    // to a Work Queues while in the TDISP
                                    // CONFIG_UNLOCKED state (T bit cleared)

    .pf_global_invalidate_support = false, // Must be false/disabled for TDISP.
                                           // Refer to PCIe Base Spec 10.5.1.2
                                           // for details
    .vf_global_invalidate_support = false, // Must be false/disabled for TDISP.
                                           // Refer to PCIe Base Spec 10.5.1.2
                                           // for details

    .rcec_assoc_bus = 0, // Bus number of RCiEP associated with the RCEC. See
                         // PCIe Base Spec 7.9.10.3 for details.

    .rciep_pf_bars_prefetch_enable = 0,    // 2-bit field. Bit0 enables prefetch
                                           // for BAR0 (PF CFG) and Bit1 enables
                                           // prefetch for BAR1 (PF Cmd Q).
                                           // Expected to be set only for debug
                                           // purposes.
    .rciep_sriov_bars_prefetch_enable = 0, // 2-bit field. Bit0 enables prefetch
                                           // for BAR0 (VF CFG) and Bit1 enables
                                           // prefetch for BAR1 (VF Cmd Q).
                                           // Expected to be set only for debug
                                           // purposes.

    .total_vfs = 32, // The number of VFs to report in the SRIOV capability
                     // TotalVFs field for the device. Refer to PCIe Base
                     // Spec 9.3.3.6 for details.

    .sriov_vf_dev_id = SDM_PCI_DEVICE_ID, // The device ID for VFs. Refer to
                                          // PCIe Base Spec 9.3.3.11 for details.
};

// ---- AccelSS PCIe RCiEP BDF ---- //
static pcie_bdf_t die0_cdedss_instance0_rciep_bdf = {.bus = D0_CDED_RCIEP_BUS, .dev = 0, .func = 0};

// ---- AccelSS PCIe RCEC BDF ---- //
static pcie_bdf_t die0_cdedss_instance0_rcec_bdf = {.bus = D0_CDED_RCEC_BUS, .dev = 0, .func = 0};

// ---- AccelSS PCIe Context ---- //
static accelip_pcie_ctxt_t die0_cdedss_instance0_pcie_ctxt = {NULL, NULL, NULL, NULL, &die0_cdedss_instance0_rciep_bdf, &die0_cdedss_instance0_rcec_bdf};

// ---- AccelSS emCPU (M7) - Memory attributes ---- //
static sdm_mem_init_t die0_cdedss_instance0_mem_init_attr = {0};

// ---- AccelSS emCPU (M7) - ECC Context ---- //
static _addressblock_0x100000_bcfg_boot_cfg_ecc_dis die0_cdedss_instance0_ecc_dis = {0};

// ---- AccelSS emCPU (M7) Context ---- //
static accelip_emcpu_ctxt_t die0_cdedss_instance0_emcpu_ctxt = {&die0_cdedss_instance0_mem_init_attr,
                                                                DIE0_CDEDSS_INSTANCE0_INT_VECTOR,
                                                                false, // Enable fence
                                                                false, // Enable I-TCM
                                                                false, // Enable D-TCM
                                                                &die0_cdedss_instance0_ecc_dis};

// ---- AccelSS Context ---- //
static accelip_ctxt_t die0_cdedss_instance0_accelip_ctxt = {&die0_cdedss_instance0_pcie_ctxt, // PCIe Ctxt
                                                            &die0_cdedss_instance0_emcpu_ctxt};

/* ---- Sub-system Context data-structures across Die's --------------------- */
static subsystem_ctxt_t ss_ctxts[] = {
    // D0 SDMSS Instance 0
    {
        {.die_instance = SOC_D0, .accel_type = ACCELERATOR_SDMSS, .accel_instance = 0},
        &die0_sdmss_instance0_vab_ctxt,
        &die0_sdmss_instance0_vab_atu_mapping_ctxt,
        &die0_sdmss_instance0_accelss_tower_attr,
        &die0_sdmss_instance0_accelss_atu_mapping_ctxt,
        NULL,                              // Pre-PCIe Cfg
        &die0_sdmss_instance0_accelip_ctxt // accel details
    },

    // D0 CDEDSS Instance 0
    {
        {.die_instance = SOC_D0, .accel_type = ACCELERATOR_CDEDSS, .accel_instance = 0},
        &die0_cdedss_instance0_vab_ctxt,
        &die0_cdedss_instance0_vab_atu_mapping_ctxt,
        NULL, // HSP will initialize CDEDSS Tower, and hence it is not needed
        &die0_cdedss_instance0_accelss_atu_mapping_ctxt,
        &die0_cdedss_instance0_pre_pcie_cfg, // Pre-PCIe Cfg
        &die0_cdedss_instance0_accelip_ctxt  // Accelerator sub-system Context
    },

    // TBD
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
