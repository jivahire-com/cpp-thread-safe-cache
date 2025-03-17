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
    ACCEL_RET_FAIL_INVALID_CPU_TYPE = (-20), // Total number of error types
    ACCEL_RET_FAIL_SS_INIT,
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
 * @brief Initialize Accelerator on SCP core
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
 * @brief Initialize accel devices on MCP core
 *
 * \b Description:
 * Initializes all the accel device instances available.
 * Accel device AP address space is mapped into ATU
 * Interrupts received from accel device is also initialized
 *
 * @retval
 *  On success, ACCEL_RET_SUCCESS
 *  On failure, failure code
 */
int32_t mcp_accelerators_init(void);

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

/**
 * @brief Populates ITCM low and high address in input arguments
 * 
 * @param[in] accel_type Accel instance
 * @param[out] low_addr Output low 32-bit ITCM address
 * @param[out] high_addr Output high 32-bit ITCM address
 */
void accel_get_itcm_addr(ACCEL_ID accel_type, uint32_t *low_addr, uint32_t *high_addr);

/**
 * @brief Populates DTCM low and high address in input arguments
 * 
 * @param[in] accel_type Accel instance
 * @param[out] low_addr Output low 32-bit DTCM address
 * @param[out] high_addr Output high 32-bit DTCM address
 */
void accel_get_dtcm_addr(ACCEL_ID accel_type, uint32_t *low_addr, uint32_t *high_addr);

/**
 * @brief Disables accel device CPU wait
 * 
 * @param accel_type Accel instance
 */
void accel_disable_cpu_wait(ACCEL_ID accel_type);
