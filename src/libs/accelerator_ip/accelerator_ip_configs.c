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
#include <accelip_id.h>          // NUM_VALID_ACCEL_ID, ACCEL_ID_SDM, ACCEL_ID_CDED
#include <ap_top_regs.h>
#include <sdm_ext_cfg_regs.h>
#include <sdm_init_knobs.h> // for MSFT_VENDOR_ID, INT_PIN_A
#include <sdm_init_struct_defaults.h>
#include <silibs_shared_knobs.h> // for ACCELIP_INIT_ECAM_ENABLED
#include <stdbool.h>             // for false, true
#include <stdint.h>              // for uint32_t
#include <vab_cded_ioss_top_regs.h>
#include <vab_sdm_top_regs.h>

/*-------------------- Symbolic Constant Macros (defines) -------------------*/

#define NUM_ACCEL_TYPE (2)

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

typedef struct
{
    ACCELIP_PCI_BASE_CLASS_CODE class_code;
    ACCELIP_PCI_SUB_CLASS_CODE sub_class_code;
    ACCELIP_PCI_DEVICE_ID pci_device_id;
    ACCELIP_PCI_DEVICE_ID subsystem_id;
} sdm_cded_class_code_t;

/*Following structure contains the default values of SDM and CDED which needs to be
overwritten from the default values provided in Silibs Knobs. More fields can be added
later if required.*/
static const sdm_cded_class_code_t sdm_cded_class_code_knobs_values[NUM_VALID_ACCEL_ID] = {
    [ACCEL_ID_SDM] =
        {
            .class_code = SDM_BASE_CLASS_CODE,
            .sub_class_code = SDM_CDED_SUB_CLASS_CODE,
            .pci_device_id = SDM_PCI_DEVICE_ID,
            .subsystem_id = SDM_PCI_DEVICE_ID,
        },
    [ACCEL_ID_CDED] =
        {
            .class_code = CDED_BASE_CLASS_CODE,
            .sub_class_code = SDM_CDED_SUB_CLASS_CODE,
            .pci_device_id = CDED_PCI_DEVICE_ID,
            .subsystem_id = CDED_PCI_DEVICE_ID,
        },
};

/*SDMSS Common structure initialization for the parameters required by sequence library API in
Silibs*/
static sdm_mem_init_cfg_t sdm_mem_init = DEFAULT_SDM_MEM_INIT_CFG_T;
static sdm_ecc_dis_init_cfg_t sdm_ecc_dis_init = DEFAULT_SDM_ECC_DIS_INIT_CFG_T;
static sdm_pre_pcie_cfg_t sdmss_pre_pcie_cfg = DEFAULT_SDM_PRE_PCIE_CFG_T;
/* TODO: ECC to be enabled: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1738671 */
static sdm_emcpu_init_cfg_t sdmss_sdm_emcpu_init_cfg = {.release_m7_halt = false,
                                                        .initvtor_byte_addr = DIE0_SDMSS_INSTANCE0_INT_VECTOR,
                                                        .enable_itcm_ecc = false,
                                                        .enable_dtcm_ecc = false};

static accelip_ss_init_knobs_t sdmss_ss_cfg = {.sys_counter_delay_value = 0, .isolation_enable = false};

/*CDEDSS Common structure initialization for the parameters required by sequence library API in
Silibs*/
static sdm_pre_pcie_cfg_t cdedss_pre_pcie_cfg = DEFAULT_SDM_PRE_PCIE_CFG_T;

static sdm_emcpu_init_cfg_t cdedss_sdm_emcpu_init_cfg = {.release_m7_halt = false,
                                                         .initvtor_byte_addr = DIE0_CDEDSS_INSTANCE0_INT_VECTOR,
                                                         .enable_itcm_ecc = false,
                                                         .enable_dtcm_ecc = false};

static accelip_ss_init_knobs_t cdedss_ss_cfg = {.sys_counter_delay_value = 0, .isolation_enable = false};

/****** Die 0 SDMSS context data start ******/
static const atu_map_entry_t die0_sdmss_atu_map = ATU_MAPPING_SDMSS_BASE(SOC_D0);

static accelip_ss_init_t die0_sdmss_init_params_ctxt = {
    .sdm_emcpu_init_cfg = &sdmss_sdm_emcpu_init_cfg,
    .fw_preload_enabled = true,
    .sdm_emcpu_fw_image_start_addr = (uintptr_t)&sdm_cded_spin_loop[0], // TODO : ADO (1728282)
    .sdm_emcpu_fw_image_size = sizeof(sdm_cded_spin_loop),              // TODO : ADO (1728282)
    .init_level = ACCELIP_INIT_ECAM_ENABLED,
    .sdm_mem_init = &sdm_mem_init,
    .sdm_ecc_disable_init = &sdm_ecc_dis_init,
    .pre_pcie_cfg = &sdmss_pre_pcie_cfg,
    .accelip_ss_cfg = &sdmss_ss_cfg};
/****** Die 0 SDMSS context data end ******/

/****** Die 1 SDMSS context data start ******/
static const atu_map_entry_t die1_sdmss_atu_map = ATU_MAPPING_SDMSS_BASE(SOC_D1);

static accelip_ss_init_t die1_sdmss_init_params_ctxt = {
    .sdm_emcpu_init_cfg = &sdmss_sdm_emcpu_init_cfg,
    .fw_preload_enabled = true,
    .sdm_emcpu_fw_image_start_addr = (uintptr_t)&sdm_cded_spin_loop[0], // TODO : ADO (1728282)
    .sdm_emcpu_fw_image_size = sizeof(sdm_cded_spin_loop),              // TODO : ADO (1728282)
    .init_level = ACCELIP_INIT_ECAM_ENABLED,
    .sdm_mem_init = &sdm_mem_init,
    .sdm_ecc_disable_init = &sdm_ecc_dis_init,
    .pre_pcie_cfg = &sdmss_pre_pcie_cfg,
    .accelip_ss_cfg = &sdmss_ss_cfg};
/****** Die 1 SDMSS context data end ******/

/****** Die 0 CDEDSS context data start ******/
static const atu_map_entry_t die0_cdedss_atu_map = ATU_MAPPING_CDEDSS_BASE(SOC_D0);

static accelip_ss_init_t die0_cdedss_init_params_ctxt = {
    .sdm_emcpu_init_cfg = &cdedss_sdm_emcpu_init_cfg,
    .fw_preload_enabled = true,
    .sdm_emcpu_fw_image_start_addr = (uintptr_t)&sdm_cded_spin_loop[0], // TODO : ADO (1728282)
    .sdm_emcpu_fw_image_size = sizeof(sdm_cded_spin_loop),              // TODO : ADO (1728282)
    .init_level = ACCELIP_INIT_ECAM_ENABLED,
    .sdm_mem_init = &sdm_mem_init,
    .sdm_ecc_disable_init = &sdm_ecc_dis_init,
    .pre_pcie_cfg = &cdedss_pre_pcie_cfg,
    .accelip_ss_cfg = &cdedss_ss_cfg};
/****** Die 0 CDEDSS context data end ******/

/****** Die 1 CDEDSS context data start ******/
static const atu_map_entry_t die1_cdedss_atu_map = ATU_MAPPING_CDEDSS_BASE(SOC_D1);
static accelip_ss_init_t die1_cdedss_init_params_ctxt = {
    .sdm_emcpu_init_cfg = &cdedss_sdm_emcpu_init_cfg,
    .fw_preload_enabled = true,
    .sdm_emcpu_fw_image_start_addr = (uintptr_t)&sdm_cded_spin_loop[0], // TODO : ADO (1728282)
    .sdm_emcpu_fw_image_size = sizeof(sdm_cded_spin_loop),              // TODO : ADO (1728282)
    .init_level = ACCELIP_INIT_ECAM_ENABLED,
    .sdm_mem_init = &sdm_mem_init,
    .sdm_ecc_disable_init = &sdm_ecc_dis_init,
    .pre_pcie_cfg = &cdedss_pre_pcie_cfg,
    .accelip_ss_cfg = &cdedss_ss_cfg};
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

void scp_accel_update_default_knobs(subsystem_ctxt_t* p_ss_ctxt)
{
    ACCEL_ID accel_ip_id = ACCEL_ID_SDM;

    if (p_ss_ctxt->accelip_metadata.accel_type == D1_ACCELIP_CDEDSS || p_ss_ctxt->accelip_metadata.accel_type == D0_ACCELIP_CDEDSS)
    {
        accel_ip_id = ACCEL_ID_CDED;
    }

    p_ss_ctxt->p_init_params->pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_base_class_code =
        sdm_cded_class_code_knobs_values[accel_ip_id].class_code;
    p_ss_ctxt->p_init_params->pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_sub_class_code =
        sdm_cded_class_code_knobs_values[accel_ip_id].sub_class_code;
    p_ss_ctxt->p_init_params->pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_device_id =
        sdm_cded_class_code_knobs_values[accel_ip_id].pci_device_id;
    p_ss_ctxt->p_init_params->pre_pcie_cfg->rciep_pci_t0_pf_cfg.pci_t0_subsystem_id =
        sdm_cded_class_code_knobs_values[accel_ip_id].subsystem_id;
}
