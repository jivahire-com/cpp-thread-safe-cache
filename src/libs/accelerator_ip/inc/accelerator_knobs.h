//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accelerator_knobs.h
 * Accelerator IP knobs specific definitions to be consumed by FW.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/
typedef enum {
    STATUS_KNOB_TRANSFER_SUCCESS = 0,
    STATUS_KNOB_TRANSFER_SKIP_NO_OVERRIDE_DATA,
    STATUS_KNOB_TRANSFER_FAIL_INVALID_BUFFER,
    STATUS_KNOB_TRANSFER_FAIL_MEMORY_OVERFLOW,
    STATUS_KNOB_TRANSFER_FAIL_SIZE_MISMATCH,
    STATUS_KNOB_TRANSFER_FAIL_INVALID_DATA_SIZE,
    STATUS_KNOB_TRANSFER_FAIL_INVALID_KNOB_NAME,
    STATUS_KNOB_TRANSFER_FAIL_INVALID_KNOB_DATA,
    STATUS_KNOB_TRANSFER_FAIL,
} knob_transfer_status_t;

typedef struct {
    const char* p_name;
    uint8_t size;
    uint8_t* p_data;
} transfer_accel_sys_info_data_t;

typedef enum {
    // Valid only for cold boot
    E_REBOOT_COLD_BOOT,
    // Valid only for warm boot
    E_REBOOT_WDT_EXPIRY,
    E_REBOOT_ITCM_UE,
    E_REBOOT_DTCM0_UE,
    E_REBOOT_DTCM1_UE,
    E_REBOOT_FIRMWARE_UPGRADE,
    E_REBOOT_ON_DEMAND_CD,
} E_REBOOT_REASON;

typedef enum {
    E_COLD_BOOT = 0,
    E_WARM_BOOT = 1,
} E_BOOT_TYPE;

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Function to download the knobs for the accelerator from SCP
 *
 * \b Description:
 * This function is invoked before initializing the accelerator 
 *
 * @param[in] accel_type - Accelerator type
 *
 * @retval
 *  No return value
 */
knob_transfer_status_t scp_download_accel_knobs(ACCEL_ID accel_type);

/**
 * @brief Set the reboot reason for the accelerator
 *
 * \b Description:
 * This function sets the reboot reason for the accelerator, which can be used later
 * to determine why the accelerator was rebooted.
 *
 * @param[in] reason - The reason for the reboot
 */
void set_reboot_reason(E_REBOOT_REASON reason);
