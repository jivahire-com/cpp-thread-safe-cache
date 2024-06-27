//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip.h
 * Accelerator IP specific definitions to be consumed by FW.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include "_addressblock_0x100000_regs.h"  // for _addressblock_0x100000_bcfg...
#include <accelerator_ip_pcie_params.h>   // for accelip_pcie_ctxt_t
#include <atu_lib.h>                      // for atu_id_t, atu_map_entry_t
#include <kng_soc_constants.h>            // for DIE_INSTANCE
#include <sdm_init.h>                     // for sdm_mem_init_t
#include <stdbool.h>                      // for bool
#include <stdint.h>                       // for int32_t, uint64_t, uint8_t
#include <sdm_init_knobs.h> // for sdm_pre_pcie_cfg_t

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/
typedef enum {
    ACCEL_RET_FAIL_SS_INIT = (-18), // Total number of error types
    ACCEL_RET_FAIL_SS_NA,
    ACCEL_RET_FAIL_DBG,
    ACCEL_RET_FAIL_CSR,
    ACCEL_RET_FAIL_RPSS_PCR,
    ACCEL_RET_FAIL_VAB,
    ACCEL_RET_FAIL_VAB_PCR,
    ACCEL_RET_FAIL_SS_PCR,
    ACCEL_RET_FAIL_SDMSS_PCR,
    ACCEL_RET_FAIL_SMMU_ENABLE,
    ACCEL_RET_FAIL_SMMU_GPBA_ENABLE,
    ACCEL_RET_FAIL_TOWER,
    ACCEL_RET_FAIL_ATU_MAP,
    ACCEL_RET_FAIL_ATU_UNMAP,
    ACCEL_RET_FAIL_ACCEL_IP,
    ACCEL_RET_FAIL_INVALID_PARAMS,
    ACCEL_RET_FAIL_INVALID_CONFIG,
    ACCEL_RET_FAIL_GENERAL,
    ACCEL_RET_SUCCESS = 0
} eACCEL_RET_CODES;

typedef enum {
    ACCELERATOR_SDMSS = 0,
    ACCELERATOR_CDEDSS,
    MAX_ACCELERATOR_TYPES
} eACCELERATOR_TYPE;

typedef struct {
    DIE_INSTANCE        die_instance;
    eACCELERATOR_TYPE   accel_type;
    uint8_t             accel_instance;
} accelip_metadata_t;

typedef struct {
    atu_id_t        ss_atu_id;
    atu_map_entry_t *p_atu_map_entry;
} atu_mapping_ctxt_t;

// Some more atttributes might get added in future
typedef struct {
    uint64_t tower_base_addr; // Absolute address for the given tower to be programmed
} tower_attr_t;

// Some more atttributes might get added in future
typedef struct {
    tower_attr_t    *p_tower_attr;
} vab_ctxt_t;

typedef struct {
    sdm_mem_init_t                                      *p_mem_init;
    uintptr_t                                           int_vtor;  // interrupt vector
    bool                                                enable_fence;
    bool                                                enable_itcm_ecc;
    bool                                                enable_dtcm_ecc;
    _addressblock_0x100000_bcfg_boot_cfg_ecc_dis       *p_cfg_ecc;
} accelip_emcpu_ctxt_t;

typedef struct {
    accelip_pcie_ctxt_t *p_pcie_ctxt;
    accelip_emcpu_ctxt_t *p_emcpu_ctxt;
} accelip_ctxt_t;

typedef struct {
    accelip_metadata_t   accelip_metadata;

    // VAB
    vab_ctxt_t          *p_vab_ctxt;
    atu_mapping_ctxt_t  *p_vab_atu_mapping_ctxt;

    // Accel Sybsystem
    tower_attr_t        *p_accelss_tower_attr;
    atu_mapping_ctxt_t  *p_accelss_atu_mapping_ctxt;

    // Silibs Data-structure for Pre-PCIe Config
    sdm_pre_pcie_cfg_t  *p_pre_pcie_cfg;

    // AccelIP details
    accelip_ctxt_t      *p_accelip_ctxt;
} subsystem_ctxt_t;

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
/**
 * @brief Initialize Accelerator
 *
 * \b Description:
 * Read fuse and based on enabled bit init respective accelerator instances
 *
 * @param[in] void
 *
 * @retval
 *  On success, ACCEL_RET_SUCCESS
 *  On failure, failure code
 */
int32_t scp_accelerators_init(void);

