//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file device_fifo_id.h
 * device ids for scf_mhu_device
 */

#pragma once

/*----------- Nested includes ------------*/


/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/**
 * @brief Fifo ID specific to device to remain uncoupled from higher Firmware layers
 *
 */
typedef enum
{
    DEVICE_FIFO_PSTATE_TLM_HW_PROD = 0,
    DEVICE_FIFO_SCP_MSG_TLM_HW_PROD = 1,
    DEVICE_FIFO_TILE_TEMP_TLM_HW_PROD = 2,
    DEVICE_FIFO_TILE_VOLT_TLM_HW_PROD = 3,
    DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD = 4,
    DEVICE_FIFO_PVT_TEMP_TLM_FW_PROD = 5,
    DEVICE_FIFO_PVT_VOLT_TLM_FW_PROD = 6,
    DEVICE_FIFO_DIMM_TEMP_TLM_FW_PROD = 7,
    DEVICE_FIFO_VR_TEMP_TLM_FW_PROD = 8,
    DEVICE_FIFO_VR_CURRENT_TLM_FW_PROD = 9,
    DEVICE_FIFO_MAX_ID = 10
} DEVICE_FIFO_ID;

/**
 * @brief Define hw fifo's first for fifo id.  allows looping of hw fifo's only
 *
 */
#define LAST_HW_PROD_FIFO_ID (DEVICE_FIFO_CORE_CURRENT_TLM_HW_PROD)
