//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip.h
 * Accelerator IP specific definitions to be consumed by FW.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <atu_lib.h>                      // for atu_id_t, atu_map_entry_t
#include <kng_soc_constants.h>            // for DIE_INSTANCE
#include <stdint.h>                       // for int32_t, uint64_t, uint8_t

#include <accelip_ss_init.h>

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
    ACCELIP_SS_INSTANCE   accel_type;
    uint8_t             accel_instance;
} accelip_metadata_t;

typedef struct {
    accelip_metadata_t   accelip_metadata;
    atu_map_entry_t     *p_accelip_atu_map;
    accelip_ss_init_t   *p_init_params;
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

