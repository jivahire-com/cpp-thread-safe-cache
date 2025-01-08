//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file power_tlm_fuse.h
 *  This library provide apis to read the power fuses for telemetry.
 * 
 *  Note :  This library can be used on both the processor MCP and SCP. 
 *  SCP have some startup dependencies, on SCP this library depends 
 *  on the fuse service to complete its initialization before it can 
 *  actually read the fuses 
 *  
 */

#pragma once

/*--------------- Includes ---------------*/
#include <stdint.h> 
#include <fpfw_status.h> // for fpfw_status_t
/*-- Symbolic Constant Macros (defines) --*/
/* DTS coeff spacing  */
#define TILE_PVT_NUM_CHANNELS_DTS 8
/*--------------- Typedefs ---------------*/

/**
 * @brief Struct for holding dts coefficient data from fuses
 */
typedef struct _dts_tlm_coeff_t
{
    uint16_t k_val;
    uint16_t y_val;
} dts_tlm_coeff_t;

/**
 * @brief API to set the staus of fuse service.
 * To be only called from the power fuse runtime init.  
 * @return None 
 */
void power_fuse_init(void);

/**
 * @brief Get the DTS coefficients for all TILE_THERMALS_SENSOR in soc die.
 *
 * @param[in] core_id - Size of the array , which will be equal number of tiles
 * @param[out] Pointer to the array to store the DTS k/y values data in..
 *
 * @return None
 */
fpfw_status_t platform_power_fuses_get_dts_coeff_tile(dts_tlm_coeff_t* dts_coeff, uint32_t count);
/**
 * @brief Get the DTS coefficients for all TOP_THERMALS_SENSOR on soc die.
 *
 * @param[in] core_id - Size of the array 
 * @param[out] Pointer to the array to store the DTS k/y values data in..
 *
 * @return None
 */
fpfw_status_t platform_power_fuses_get_dts_coeff_soctop(dts_tlm_coeff_t* dts_coeff, uint32_t count);

