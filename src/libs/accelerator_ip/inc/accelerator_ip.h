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
#include <accel_intr.h>                   // for E_ACCEL_INTR_INIT_CONFIG
#include <atu_lib.h>                      // for atu_id_t, atu_map_entry_t
#include <fpfw_status.h>                  // for fpfw_status_t
#include <icc_platform_defines.h>         // for large_fifo_mailbox_msg_header
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

typedef struct _uefi_boot_stat_mailbox_msg
{
	large_fifo_mailbox_msg_header hdr;
    uint32_t data;
} uefi_boot_stat_mailbox_msg;


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
 * @brief Warm reset the EMCPU of the accelerator
 * \b Description:
 * This function is invoked to reset the EMCPU of the accelerator.
 * It performs the necessary steps to reset the EMCPU and
 * invokes the pre and post reset callbacks if provided.
 * @param[in] accel_type - Accelerator type
 * @return
 *  No return value
 */
void accel_core_warm_reset(ACCEL_ID accel_type);

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

/**
 * @brief This API will read KNOB and return true if Accelerator
 * isolation is enabled otherwise false
 *
 * @param[in] accel_type Accel instance
 *
 * @retval Return True if Isolation enabled otherwise False
 */
bool accel_is_isolation_enabled(ACCEL_ID accel_type);

/**
 * @brief Setup SCP to receive boot status code from accel cores
 *
 * @param[in] accel_type Accel instance
 *
 * @return fpfw_status_t
 */
fpfw_status_t accel_setup_boot_status_code(ACCEL_ID accel_type);

/**
 * @brief Start the timer which expects boot status code from accel cores
 *
 * @param accel_type Accel instance
 * @return fpfw_status_t
 */
fpfw_status_t accel_start_boot_status_timer(ACCEL_ID accel_type);

/**
 * @brief This function is used to suspend the accelerator core
 *
 * @param accel_type Accel instance
 *
 * @return No return value
 */
void accel_core_suspend(ACCEL_ID accel_type);

/**
 * @brief This function will notify the accelerators that UEFI boot is done.
 *
 * @param[in] none
 * @return status
 */
int32_t notify_accelerators_uefi_boot(void);

/**
 * @brief Call back for notify_accelerators UEFI boot function
 *
 * @param[in] context accelerator ID
 * @param[in] status Status of the ICC send operation
 * @return none
 */
void notify_accelerators_uefi_boot_cb(void* context, fpfw_status_t status);

/**
 * @brief Non-block call Completes the request passed if given accel is up and running.
 * If the given accel is not booted then it stores the given SOS DFWK request and
 * complete is later on when it gets boot complete notification from the given accelerator
 * 
 * @param[in] accel_type Accel instance
 * @param[in] p_request Pointer to the SOS DFWK async request header
 * @return none
 */
void accel_boot_status_wait_boot_complete(ACCEL_ID accel_type, PDFWK_ASYNC_REQUEST_HEADER p_request);
