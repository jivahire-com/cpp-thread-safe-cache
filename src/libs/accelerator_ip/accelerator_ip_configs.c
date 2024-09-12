//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip_configs.c
 * This file provides the configurations needed to initialize the Accelerator
 * IP(s) available on the SoC.
 */

/*-------------------------------- Includes ---------------------------------*/
#include "accelerator_ip.h"             // for subsystem_ctxt_t
#include "accelerator_ip_bcfg_params.h" // for BCFG_BOOT_CFG_TID_CTRL_GIC_TID
#include "accelip_ss_init.h"            // for D0_ACCELIP_CDEDSS, D0_ACCELI...
#include "accelip_ss_init_knobs.h"      // for accelip_ss_init_knobs_t
#include "atu_lib.h"                    // for atu_map_entry_t
#include "kng_atu_mappings.h"           // for ATU_MAPPING_CDEDSS_BASE, ATU...
#include "kng_soc_constants.h"          // for SOC_D0
#include "sdm_init.h"                   // for sdm_mem_init_t
#include "silibs_common.h"              // for ARRAY_SIZE

#include <accelerator_ip_priv.h> // for get_accelerator_ctxt
#include <ap_top_regs.h>
#include <sdm_ext_cfg_regs.h>
#include <sdm_init_knobs.h>      // for MSFT_VENDOR_ID, INT_PIN_A
#include <silibs_shared_knobs.h> // for ACCELIP_INIT_ECAM_ENABLED
#include <stdbool.h>             // for false, true
#include <stdint.h>              // for uint32_t
#include <vab_cded_ioss_top_regs.h>
#include <vab_sdm_top_regs.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/
#define HIGH_ADDR(x) (uintptr_t)((x >> 32) & 0xFFFFFFFF)
#define LOW_ADDR(x)  (uintptr_t)(x & 0xFFFFFFFF)

#define SOC_D0_SDM_EMCPU_ITCM_ADDR \
    (AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS + SDM_EXT_CFG_EMCPU_TCM_ITCM_ADDRESS)
#define SOC_D0_SDM_EMCPU_DTCM_ADDR \
    (AP_TOP_D0_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS)

#define SOC_D0_CDED_SDM_EMCPU_ITCM_ADDR \
    (AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_CDEDSS_ADDRESS + SDM_EXT_CFG_EMCPU_TCM_ITCM_ADDRESS)
#define SOC_D0_CDED_SDM_EMCPU_DTCM_ADDR \
    (AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_CDEDSS_ADDRESS + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS)

#define SOC_D1_SDM_EMCPU_ITCM_ADDR \
    (AP_TOP_D1_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS + SDM_EXT_CFG_EMCPU_TCM_ITCM_ADDRESS)
#define SOC_D1_SDM_EMCPU_DTCM_ADDR \
    (AP_TOP_D1_VAB_SDM_ADDRESS + VAB_SDM_TOP_SDMSS_ADDRESS + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS)

#define SOC_D1_CDED_SDM_EMCPU_ITCM_ADDR \
    (AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_CDEDSS_ADDRESS + SDM_EXT_CFG_EMCPU_TCM_ITCM_ADDRESS)
#define SOC_D1_CDED_SDM_EMCPU_DTCM_ADDR \
    (AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_CDEDSS_ADDRESS + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS)

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

// This is a basic a vector table and spin loop at reset code.
// This is implemented for FPGA, where elf is not preloaded.
// This should be removed once we have the code download sequence pulled in. ADO (1728282)
static const uint32_t sdm_cded_spin_loop[] = {
    0x20000200, 0x00080049, 0x00080059, 0x00080069, // 0x80000 - 0x8000F
    0x00080079, 0x00080089, 0x00080099, 0x00000000, // 0x80010 - 0x8001F
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // 0x80020 - 0x8002F
    0x00000000, 0x00000000, 0x00000000, 0x00000000, // 0x80030 - 0x8003F
    0x00000000, 0x00000000, 0xF05F2500, 0x60055000, // 0x80040 - 0x8004F
    0xE7FC3501, 0x00000000, 0x35012500, 0xBF00E7FD, // 0x80050 - 0x8005F
    0x00000000, 0x00000000, 0x35012500, 0xBF00E7FD, // 0x80060 - 0x8006F
    0x00000000, 0x00000000, 0x35012500, 0xBF00E7FD, // 0x80070 - 0x8007F
    0x00000000, 0x00000000, 0x35012500, 0xBF00E7FD, // 0x80080 - 0x8008F
    0x00000000, 0x00000000, 0x35012500, 0xBF00E7FD, // 0x80090 - 0x8009F
};

/****** Die 0 SDMSS context data start ******/
static const atu_map_entry_t die0_sdmss_atu_map = ATU_MAPPING_SDMSS_BASE(SOC_D0);

/* TODO: ECC to be enabled: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1738671 */
static sdm_emcpu_init_cfg_t die0_sdmss_sdm_emcpu_init_cfg = {.release_m7_halt = false,
                                                             .initvtor_byte_addr = DIE0_SDMSS_INSTANCE0_INT_VECTOR,
                                                             .enable_itcm_ecc = false,
                                                             .enable_dtcm_ecc = false};
static sdm_mem_init_t die0_sdmss_sdm_mem_init = {.lstrg = false, .itcm = false, .d0tcm = false, .d1tcm = false, .ecu = false};
static accelip_ss_init_knobs_t die0_sdmss_ss_cfg = {.sys_counter_delay_value = 0, .isolation_enable = false};
static sdm_pre_pcie_cfg_t die0_sdmss_pre_pcie_cfg = {
    // PF RCiEP PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                       // Revision ID of the device
        .pci_t0_base_class_code = SDM_BASE_CLASS_CODE, // Base class code of the device
        .pci_t0_sub_class_code = SDM_SUB_CLASS_CODE,   // Sub class code of the device
        .pci_t0_subsystem_id = SDM_PCI_DEVICE_ID,      // Subsystem ID of the device
        .pci_t0_subsystem_vendor_id = MSFT_VENDOR_ID,  // Subsystem vendor ID of the device
        .pci_t0_device_id = SDM_PCI_DEVICE_ID,         // PCI device ID
        .pci_t0_vendor_id = MSFT_VENDOR_ID,            // PCI vendor ID
        .pci_t0_int_pin = INT_PIN_A,                   // Legacy interrupt pin setting
    },

    // VF RCiEP PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                  // Revision ID of the device
        .pci_t0_subsystem_id = SDM_PCI_DEVICE_ID, // Subsystem ID of the device
    },

    // RCEC PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                        // Revision ID of the device
        .pci_t0_base_class_code = RCEC_BASE_CLASS_CODE, // Base class code of the device
        .pci_t0_sub_class_code = RCEC_SUB_CLASS_CODE,   // Sub class code of the device
        .pci_t0_subsystem_id = RCEC_PCI_DEVICE_ID,      // Subsystem ID of the device
        .pci_t0_subsystem_vendor_id = MSFT_VENDOR_ID,   // Subsystem vendor ID of the device
        .pci_t0_device_id = RCEC_PCI_DEVICE_ID,         // PCI device ID
        .pci_t0_vendor_id = MSFT_VENDOR_ID,             // PCI vendor ID
        .pci_t0_int_pin = INT_PIN_A,                    // Legacy interrupt pin setting
    },

    .dti_v3_protocol = DTI_ATS_V3, // DTI protocol to use (v2 or v3)

    // Response to config access timeout
    .pf_cfg_timeout_response = CRS_RESPONSE,
    .vf_cfg_timeout_response = CRS_RESPONSE,

    .tee_io_supp = true, // TODO: ADO 1885063: Enable TEE IO support capability

    .gic_tid = BCFG_BOOT_CFG_TID_CTRL_GIC_TID,           // GIC TID
    .smmu_dti_tid = BCFG_BOOT_CFG_TID_CTRL_SMMU_DTI_TID, // SMMU DTI TID

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

static accelip_ss_init_t die0_sdmss_init_params_ctxt = {
    .sdm_emcpu_init_cfg = &die0_sdmss_sdm_emcpu_init_cfg,
    .fw_preload_enabled = true,
    .sdm_emcpu_fw_image_start_addr = (uintptr_t)&sdm_cded_spin_loop[0], // TODO : ADO (1728282)
    .sdm_emcpu_fw_image_size = sizeof(sdm_cded_spin_loop),              // TODO : ADO (1728282)
    .init_level = ACCELIP_INIT_ECAM_ENABLED,
    .sdm_mem_init = &die0_sdmss_sdm_mem_init,
    .pre_pcie_cfg = &die0_sdmss_pre_pcie_cfg,
    .accelip_ss_cfg = &die0_sdmss_ss_cfg};
/****** Die 0 SDMSS context data end ******/

/****** Die 1 SDMSS context data start ******/
static const atu_map_entry_t die1_sdmss_atu_map = ATU_MAPPING_SDMSS_BASE(SOC_D1);

/* TODO: ECC to be enabled: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1738671 */
static sdm_emcpu_init_cfg_t die1_sdmss_sdm_emcpu_init_cfg = {.release_m7_halt = false,
                                                             .initvtor_byte_addr = DIE0_SDMSS_INSTANCE0_INT_VECTOR,
                                                             .enable_itcm_ecc = false,
                                                             .enable_dtcm_ecc = false};
static sdm_mem_init_t die1_sdmss_sdm_mem_init = {.lstrg = false, .itcm = false, .d0tcm = false, .d1tcm = false, .ecu = false};
static accelip_ss_init_knobs_t die1_sdmss_ss_cfg = {.sys_counter_delay_value = 0, .isolation_enable = false};
static sdm_pre_pcie_cfg_t die1_sdmss_pre_pcie_cfg = {
    // PF RCiEP PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                       // Revision ID of the device
        .pci_t0_base_class_code = SDM_BASE_CLASS_CODE, // Base class code of the device
        .pci_t0_sub_class_code = SDM_SUB_CLASS_CODE,   // Sub class code of the device
        .pci_t0_subsystem_id = SDM_PCI_DEVICE_ID,      // Subsystem ID of the device
        .pci_t0_subsystem_vendor_id = MSFT_VENDOR_ID,  // Subsystem vendor ID of the device
        .pci_t0_device_id = SDM_PCI_DEVICE_ID,         // PCI device ID
        .pci_t0_vendor_id = MSFT_VENDOR_ID,            // PCI vendor ID
        .pci_t0_int_pin = INT_PIN_A,                   // Legacy interrupt pin setting
    },

    // VF RCiEP PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                  // Revision ID of the device
        .pci_t0_subsystem_id = SDM_PCI_DEVICE_ID, // Subsystem ID of the device
    },

    // RCEC PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                        // Revision ID of the device
        .pci_t0_base_class_code = RCEC_BASE_CLASS_CODE, // Base class code of the device
        .pci_t0_sub_class_code = RCEC_SUB_CLASS_CODE,   // Sub class code of the device
        .pci_t0_subsystem_id = RCEC_PCI_DEVICE_ID,      // Subsystem ID of the device
        .pci_t0_subsystem_vendor_id = MSFT_VENDOR_ID,   // Subsystem vendor ID of the device
        .pci_t0_device_id = RCEC_PCI_DEVICE_ID,         // PCI device ID
        .pci_t0_vendor_id = MSFT_VENDOR_ID,             // PCI vendor ID
        .pci_t0_int_pin = INT_PIN_A,                    // Legacy interrupt pin setting
    },

    .dti_v3_protocol = DTI_ATS_V3, // DTI protocol to use (v2 or v3)

    // Response to config access timeout
    .pf_cfg_timeout_response = CRS_RESPONSE,
    .vf_cfg_timeout_response = CRS_RESPONSE,

    .tee_io_supp = true, // TODO: ADO 1885063: Enable TEE IO support capability

    .gic_tid = BCFG_BOOT_CFG_TID_CTRL_GIC_TID,           // GIC TID
    .smmu_dti_tid = BCFG_BOOT_CFG_TID_CTRL_SMMU_DTI_TID, // SMMU DTI TID

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

static accelip_ss_init_t die1_sdmss_init_params_ctxt = {
    .sdm_emcpu_init_cfg = &die1_sdmss_sdm_emcpu_init_cfg,
    .fw_preload_enabled = true,
    .sdm_emcpu_fw_image_start_addr = (uintptr_t)&sdm_cded_spin_loop[0], // TODO : ADO (1728282)
    .sdm_emcpu_fw_image_size = sizeof(sdm_cded_spin_loop),              // TODO : ADO (1728282)
    .init_level = ACCELIP_INIT_ECAM_ENABLED,
    .sdm_mem_init = &die1_sdmss_sdm_mem_init,
    .pre_pcie_cfg = &die1_sdmss_pre_pcie_cfg,
    .accelip_ss_cfg = &die1_sdmss_ss_cfg};
/****** Die 1 SDMSS context data end ******/

/****** Die 0 CDEDSS context data start ******/
static const atu_map_entry_t die0_cdedss_atu_map = ATU_MAPPING_CDEDSS_BASE(SOC_D0);

/* TODO: ECC to be enabled: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1738671 */
static sdm_emcpu_init_cfg_t die0_cdedss_sdm_emcpu_init_cfg = {.release_m7_halt = false,
                                                              .initvtor_byte_addr = DIE0_CDEDSS_INSTANCE0_INT_VECTOR,
                                                              .enable_itcm_ecc = false,
                                                              .enable_dtcm_ecc = false};
static sdm_mem_init_t die0_cdedss_sdm_mem_init = {.lstrg = false, .itcm = false, .d0tcm = false, .d1tcm = false, .ecu = false};
static accelip_ss_init_knobs_t die0_cdedss_ss_cfg = {.sys_counter_delay_value = 0, .isolation_enable = false};
static sdm_pre_pcie_cfg_t die0_cdedss_pre_pcie_cfg = {
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
    {
        .pci_t0_revision_id = 0,                   // Revision ID of the device
        .pci_t0_subsystem_id = CDED_PCI_DEVICE_ID, // Subsystem ID of the device
    },

    // RCEC PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                        // Revision ID of the device
        .pci_t0_base_class_code = RCEC_BASE_CLASS_CODE, // Base class code of the device
        .pci_t0_sub_class_code = RCEC_SUB_CLASS_CODE,   // Sub class code of the device
        .pci_t0_subsystem_id = RCEC_PCI_DEVICE_ID,      // Subsystem ID of the device
        .pci_t0_subsystem_vendor_id = MSFT_VENDOR_ID,   // Subsystem vendor ID of the device
        .pci_t0_device_id = RCEC_PCI_DEVICE_ID,         // PCI device ID
        .pci_t0_vendor_id = MSFT_VENDOR_ID,             // PCI vendor ID
        .pci_t0_int_pin = INT_PIN_A,                    // Legacy interrupt pin setting
    },

    .dti_v3_protocol = DTI_ATS_V3, // DTI protocol to use (v2 or v3)

    // Response to config access timeout
    .pf_cfg_timeout_response = CRS_RESPONSE,
    .vf_cfg_timeout_response = CRS_RESPONSE,

    .tee_io_supp = true, // TODO: ADO 1885063: Enable TEE IO support capability

    .gic_tid = BCFG_BOOT_CFG_TID_CTRL_GIC_TID,           // GIC TID
    .smmu_dti_tid = BCFG_BOOT_CFG_TID_CTRL_SMMU_DTI_TID, // SMMU DTI TID

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

    .sriov_vf_dev_id = CDED_PCI_DEVICE_ID, // The device ID for VFs. Refer to
                                           // PCIe Base Spec 9.3.3.11 for details.
};

static accelip_ss_init_t die0_cdedss_init_params_ctxt = {
    .sdm_emcpu_init_cfg = &die0_cdedss_sdm_emcpu_init_cfg,
    .fw_preload_enabled = true,
    .sdm_emcpu_fw_image_start_addr = (uintptr_t)&sdm_cded_spin_loop[0], // TODO : ADO (1728282)
    .sdm_emcpu_fw_image_size = sizeof(sdm_cded_spin_loop),              // TODO : ADO (1728282)
    .init_level = ACCELIP_INIT_ECAM_ENABLED,
    .sdm_mem_init = &die0_cdedss_sdm_mem_init,
    .pre_pcie_cfg = &die0_cdedss_pre_pcie_cfg,
    .accelip_ss_cfg = &die0_cdedss_ss_cfg};
/****** Die 0 CDEDSS context data end ******/

/****** Die 1 CDEDSS context data start ******/
static const atu_map_entry_t die1_cdedss_atu_map = ATU_MAPPING_CDEDSS_BASE(SOC_D1);

/* TODO: ECC to be enabled: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1738671 */
static sdm_emcpu_init_cfg_t die1_cdedss_sdm_emcpu_init_cfg = {.release_m7_halt = false,
                                                              .initvtor_byte_addr = DIE0_CDEDSS_INSTANCE0_INT_VECTOR,
                                                              .enable_itcm_ecc = false,
                                                              .enable_dtcm_ecc = false};
static sdm_mem_init_t die1_cdedss_sdm_mem_init = {.lstrg = false, .itcm = false, .d0tcm = false, .d1tcm = false, .ecu = false};
static accelip_ss_init_knobs_t die1_cdedss_ss_cfg = {.sys_counter_delay_value = 0, .isolation_enable = false};
static sdm_pre_pcie_cfg_t die1_cdedss_pre_pcie_cfg = {
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
    {
        .pci_t0_revision_id = 0,                   // Revision ID of the device
        .pci_t0_subsystem_id = CDED_PCI_DEVICE_ID, // Subsystem ID of the device
    },

    // RCEC PCI T0 Cfg
    {
        .pci_t0_revision_id = 0,                        // Revision ID of the device
        .pci_t0_base_class_code = RCEC_BASE_CLASS_CODE, // Base class code of the device
        .pci_t0_sub_class_code = RCEC_SUB_CLASS_CODE,   // Sub class code of the device
        .pci_t0_subsystem_id = RCEC_PCI_DEVICE_ID,      // Subsystem ID of the device
        .pci_t0_subsystem_vendor_id = MSFT_VENDOR_ID,   // Subsystem vendor ID of the device
        .pci_t0_device_id = RCEC_PCI_DEVICE_ID,         // PCI device ID
        .pci_t0_vendor_id = MSFT_VENDOR_ID,             // PCI vendor ID
        .pci_t0_int_pin = INT_PIN_A,                    // Legacy interrupt pin setting
    },

    .dti_v3_protocol = DTI_ATS_V3, // DTI protocol to use (v2 or v3)

    // Response to config access timeout
    .pf_cfg_timeout_response = CRS_RESPONSE,
    .vf_cfg_timeout_response = CRS_RESPONSE,

    .tee_io_supp = true, // TODO: ADO 1885063: Enable TEE IO support capability

    .gic_tid = BCFG_BOOT_CFG_TID_CTRL_GIC_TID,           // GIC TID
    .smmu_dti_tid = BCFG_BOOT_CFG_TID_CTRL_SMMU_DTI_TID, // SMMU DTI TID

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

    .sriov_vf_dev_id = CDED_PCI_DEVICE_ID, // The device ID for VFs. Refer to
                                           // PCIe Base Spec 9.3.3.11 for details.
};

static accelip_ss_init_t die1_cdedss_init_params_ctxt = {
    .sdm_emcpu_init_cfg = &die1_cdedss_sdm_emcpu_init_cfg,
    .fw_preload_enabled = true,
    .sdm_emcpu_fw_image_start_addr = (uintptr_t)&sdm_cded_spin_loop[0], // TODO : ADO (1728282)
    .sdm_emcpu_fw_image_size = sizeof(sdm_cded_spin_loop),              // TODO : ADO (1728282)
    .init_level = ACCELIP_INIT_ECAM_ENABLED,
    .sdm_mem_init = &die1_cdedss_sdm_mem_init,
    .pre_pcie_cfg = &die1_cdedss_pre_pcie_cfg,
    .accelip_ss_cfg = &die1_cdedss_ss_cfg};
/****** Die 1 CDEDSS context data end ******/

/* ---- Sub-system Context data-structures across Die's --------------------- */
static subsystem_ctxt_t ss_ctxts[] = {
    // D0 SDMSS Instance 0
    {{.die_instance = SOC_D0, .accel_type = D0_ACCELIP_SDMSS, .accel_instance = 0},
     &die0_sdmss_atu_map,
     &die0_sdmss_init_params_ctxt,
     {.itcm_load_addr_low = LOW_ADDR(SOC_D0_SDM_EMCPU_ITCM_ADDR),
      .itcm_load_addr_high = HIGH_ADDR(SOC_D0_SDM_EMCPU_ITCM_ADDR),
      .dtcm_load_addr_low = LOW_ADDR(SOC_D0_SDM_EMCPU_DTCM_ADDR),
      .dtcm_load_addr_high = HIGH_ADDR(SOC_D0_SDM_EMCPU_DTCM_ADDR)}},

    // D0 CDEDSS Instance 0
    {{.die_instance = SOC_D0, .accel_type = D0_ACCELIP_CDEDSS, .accel_instance = 0},
     &die0_cdedss_atu_map,
     &die0_cdedss_init_params_ctxt,
     {.itcm_load_addr_low = LOW_ADDR(SOC_D0_CDED_SDM_EMCPU_ITCM_ADDR),
      .itcm_load_addr_high = HIGH_ADDR(SOC_D0_CDED_SDM_EMCPU_ITCM_ADDR),
      .dtcm_load_addr_low = LOW_ADDR(SOC_D0_CDED_SDM_EMCPU_DTCM_ADDR),
      .dtcm_load_addr_high = HIGH_ADDR(SOC_D0_CDED_SDM_EMCPU_DTCM_ADDR)}},

    // D1 SDMSS Instance 0
    {{.die_instance = SOC_D1, .accel_type = D1_ACCELIP_SDMSS, .accel_instance = 0},
     &die1_sdmss_atu_map,
     &die1_sdmss_init_params_ctxt,
     {.itcm_load_addr_low = LOW_ADDR(SOC_D1_SDM_EMCPU_ITCM_ADDR),
      .itcm_load_addr_high = HIGH_ADDR(SOC_D1_SDM_EMCPU_ITCM_ADDR),
      .dtcm_load_addr_low = LOW_ADDR(SOC_D1_SDM_EMCPU_DTCM_ADDR),
      .dtcm_load_addr_high = HIGH_ADDR(SOC_D1_SDM_EMCPU_DTCM_ADDR)}},

    // D1 CDEDSS Instance 0
    {{.die_instance = SOC_D1, .accel_type = D1_ACCELIP_CDEDSS, .accel_instance = 0},
     &die1_cdedss_atu_map,
     &die1_cdedss_init_params_ctxt,
     {.itcm_load_addr_low = LOW_ADDR(SOC_D1_CDED_SDM_EMCPU_ITCM_ADDR),
      .itcm_load_addr_high = HIGH_ADDR(SOC_D1_CDED_SDM_EMCPU_ITCM_ADDR),
      .dtcm_load_addr_low = LOW_ADDR(SOC_D1_CDED_SDM_EMCPU_DTCM_ADDR),
      .dtcm_load_addr_high = HIGH_ADDR(SOC_D1_CDED_SDM_EMCPU_DTCM_ADDR)}},
};
/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
subsystem_ctxt_t* get_accelerator_ctxt(uint32_t* accel_instance_size)
{
    *accel_instance_size = ARRAY_SIZE(ss_ctxts);
    return ss_ctxts;
}
