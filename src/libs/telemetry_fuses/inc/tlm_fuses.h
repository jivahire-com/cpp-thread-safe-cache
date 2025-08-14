//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file tlm_fuses.h
 *  This library provide apis to read the fuses for telemetry.
 *
 *  Note: This library can be used on both the MCP and SCP,
 *        see the runtime init component for dependency details.
 */

#pragma once

/*--------------- Includes ---------------*/

#include <stdint.h>
#include <fpfw_status.h>

/*-- Symbolic Constant Macros (defines) --*/

/**
 *  Helpers for handling the DTS coefficients.
 */

#define TLM_DTS_K_COEFF_FUSED_TEMP(fused_k) (-1.0F * (float)fused_k)
#define TLM_DTS_Y_COEFF_FUSED_TEMP(fused_y) ((float)fused_y)

//
// Default values from the datasheet for the sensors, section 3.1 "Coefficients  for Deep N-Well".
// https://microsoft.sharepoint.com/:b:/r/teams/Kingsgate/Shared%20Documents/Third%20Party%20IP/SNPS/Sensors%20%26%20Monitors%20(formerly%20from%20Moortec)/DTS/N3P/dwc_sensors_dts2_tsmcn3p_databook.pdf?csf=1&web=1&e=OhWey1
//
// Rounded and unsigned to reflect as if the values are from the fuses, see above macros for the appropriate sign application.
//
#define DEFAULT_DTS_FUSED_Y_VAL (648)
#define DEFAULT_DTS_FUSED_K_VAL (281)

/**
 * @brief To obtain the temperature value, the following equation should be applied to the dout output of the DTS.
 * Temperature = ((( 𝒅𝒐𝒖𝒕 /16384 ) ∗ 𝒀+𝑲) * 10) + 0.5 [dC]. We want to round up so we add half a dC.
 * Reference : Synopsys Cores Sensors  Distributed Thermal Sensor (Series 2) section 6.2
 */

#ifndef TLM_FUSE_DOUT_TO_TEMP_DC
    #define TLM_FUSE_DOUT_TO_TEMP_DC(dout, fused_k, fused_y) \
    ((uint16_t)(((((dout) / 16384.0F) * TLM_DTS_Y_COEFF_FUSED_TEMP(fused_y) + TLM_DTS_K_COEFF_FUSED_TEMP(fused_k)) * 10.0F) + 0.5F))
#endif

#ifndef TLM_FUSE_TEMP_DC_TO_DOUT
    #define TLM_FUSE_TEMP_DC_TO_DOUT(temp, fused_k, fused_y) \
    ((uint16_t)(16384.0F * ((((double)(temp) - 0.5F) / 10.0F - TLM_DTS_K_COEFF_FUSED_TEMP(fused_k)) / TLM_DTS_Y_COEFF_FUSED_TEMP(fused_y)) + 0.5F))
#endif

#ifndef TLM_FUSE_TEMP_CEL_2_DOUT
    #define TLM_FUSE_TEMP_CEL_2_DOUT(temp, fused_k, fused_y) \
    ((uint16_t)((16384.0F) * (((temp) - TLM_DTS_K_COEFF_FUSED_TEMP(fused_k)) / TLM_DTS_Y_COEFF_FUSED_TEMP(fused_y))))
#endif

/* DTS coeff spacing */
#define TILE_PVT_NUM_CHANNELS_DTS 8


/**
 *  Helpers for handling Electronic Chip Identifier (ECID).
 */

 #define ECID_WAFER_LOT_NUMBER_CHAR_SIZE (9)

/*--------------- Typedefs ---------------*/

/**
 * @brief Struct for holding dts coefficient data from fuses
 */
typedef struct _dts_tlm_coeff_t
{
    uint16_t k_val;
    uint16_t y_val;
} dts_tlm_coeff_t;

//
// See https://dev.azure.com/smpe-peak/Fuse/_wiki/wikis/Fuse.wiki/42/-Design-Gen2-ECID-definition
// for more information on the ECID structure.
//
typedef struct _ecid_t
{
    // Plus one for the null terminator
    char wafer_lot_num[ECID_WAFER_LOT_NUMBER_CHAR_SIZE + 1];
    uint8_t wafer_num;
    uint8_t x_coord;
    uint8_t y_coord;
} ecid_t;

/*------ Function Prototypes ------------*/

/**
 * @brief Initialize the library. Captures status of dependencies being initialized.
 *
 * @return None
 */
void tlm_fuses_init(void);

/**
 * @brief Get the DTS coefficients for all TILE_THERMALS_SENSOR in soc die.
 *
 * @param[out] dts_coeff - Pointer to memory to store the fuse data in.
 * @param[in] count Number of elements that can fit in dts_coeff.
 *
 * @return FPFW_STATUS_SUCCESS on success, error code otherwise.
 */
fpfw_status_t tlm_fuses_get_dts_coeff_tile(dts_tlm_coeff_t* dts_coeff, uint32_t count);

/**
 * @brief Get the DTS coefficients for all TOP_THERMALS_SENSOR on soc die.
 *
 * @param[out] dts_coeff - Pointer to memory to store the fuse data in.
 * @param[in] count Number of elements that can fit in dts_coeff.
 *
 * @return FPFW_STATUS_SUCCESS on success, error code otherwise.
 */
fpfw_status_t tlm_fuses_get_dts_coeff_soctop(dts_tlm_coeff_t* dts_coeff, uint32_t count);

/**
 * @brief Get the Electronic Chip Identifier (ECID) from fuses.
 *
 * @param[out] ecid - Pointer to the ecid_t structure to store the ECID.
 * 
 * @return FPFW_STATUS_SUCCESS on success, error code otherwise.
 */
fpfw_status_t tlm_fuses_get_ecid(ecid_t* ecid);
