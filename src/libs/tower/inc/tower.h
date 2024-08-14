//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file tower.h
 *  COnfigures various SOC and subsystem towers
 */

#pragma once

/*--------------- Includes ---------------*/

#include <MboxPrimitives.h> // for FPFW_MBX_PAYLOAD, FpFwMailbox...
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 *
 *    The function configures various SOC towers (non-key towers) 
 *    It configures the following:
 *              - Fabric tower
 *              - Peripheral tower 
 *              - VAB towers for all VAB instances
 *              - RPSS towers for all RPSS instances
 *              - SDMSS tower for all SDMSS instances
 *              - IOSS tower for IOSS instance on die0 only
 *    This should be called after the mesh has been configured and HSP FW has
 *    setup the system SMMU.
 *  
 *    @param[in] die_num
 *              Current die numer
 *    @param[in] p_mbox_prim_ctx
 *              Pointer to mailbox primitives API structure
 * 
 *    @retval none
 */
void tower_init(uint8_t die_num, FPFW_MBX_PRIMITIVE_CTX* p_mbox_prim_ctx);