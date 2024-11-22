//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_ip.h
 * Accelerator IP specific definitions to be consumed by FW.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <accelip_id.h>                   // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <atu_lib.h>                      // for atu_id_t, atu_map_entry_t
#include <kng_soc_constants.h>            // for DIE_INSTANCE
#include <stdint.h>                       // for int32_t, uint64_t, uint8_t

#include <accelip_ss_init.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/
typedef enum {
    ACCEL_RET_FAIL_SS_INIT = (-19), // Total number of error types
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
    ACCEL_RET_FAIL_INTR_INIT,
    ACCEL_RET_SUCCESS = 0
} eACCEL_RET_CODES;

typedef struct {
    uintptr_t itcm_load_addr_low;
    uintptr_t itcm_load_addr_high;
    uintptr_t dtcm_load_addr_low;
    uintptr_t dtcm_load_addr_high;
} accelip_mem_info_t;

typedef struct {
    DIE_INSTANCE        die_instance;
    ACCELIP_SS_INSTANCE   accel_type;
    uint8_t             accel_instance;
} accelip_metadata_t;

typedef struct {
    accelip_metadata_t   accelip_metadata;
    const atu_map_entry_t  *p_accelip_atu_map;
    accelip_ss_init_t   *p_init_params;
    accelip_mem_info_t mem_info;
} subsystem_ctxt_t;

typedef void (*crash_dump_cb_t)(void *);

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
/**
 * @brief Get ATU mapped address for ext_cfg based on Accel Type
 *
 *
 * @param[in] accel_type : Accelerator type : SDM / CDED
 *
 * @retval ATU mapped Accel Ext Cfg address
 */
uint32_t accelerator_ip_get_atu_mapped_cfg_address(ACCEL_ID accel_type);

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

/**
 * @brief Accelerator isolation control
 *
 * \b Description:
 * Accelerators are isolated by default. Based on fuses and knobs, configure isolation for accelerators
 *
 * @param[in] void
 *
 * @retval
 *  On success, ACCEL_RET_SUCCESS
 *  On failure, failure code
 */
int32_t scp_accelerators_isolation_control(void);

/**
 * @brief Accelerator EMCPU recovery flow
 *
 * \b Description:
 * This function is invoked when the EMCPU encounters a fatal and has to be reset
 *
 * @param[in] accel_type - Accelerator the EMCPU belongs to
 * 
 * @param[in] cb_fun - Callback to be invoked when recovery sequence is completed
 * 
 * @param[in] cb_ctx - Context to be passed to the callback function
 *
 * @retval
 *  No return value
 */
void scp_accelerators_emcpu_reset(ACCEL_ID accel_type, crash_dump_cb_t cb_fun, void *cb_ctx);

/**
 * @brief Function to overwrite the default values of knobs provided by Silibs
 *
 * \b Description:
 * This function is invoked before initializing the accelerator 
 *
 * @param[in] p_ss_ctxt - Pointer to the structure which holds the information required
 * to initialize the particular accelerator 
 * 
 * @retval
 *  No return value
 */
void scp_accel_update_default_knobs(subsystem_ctxt_t* p_ss_ctxt);

/**
 * @brief Converts the accel instance to accel id
 *
 * \b Description:
 * This function converts the accel instance to accel id
 *
 * @param[in] accel_type - Accel instance
 *
 * @retval
 *  Corresponding accel id for the provided accel type
 */
ACCEL_ID get_accelip_type(ACCELIP_SS_INSTANCE accel_type);
