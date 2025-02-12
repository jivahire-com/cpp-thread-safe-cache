//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_atu_map.h
 * Header file to support implementations of ddr ATU mapping.
 */


/*------------- Includes -----------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Function to map ATU for DDRSS Config Space
 * 
 * @param die_num - die number
 * @return uintptr_t - mapped address
 */
uintptr_t ddrss_atu_map_cfg_space(uint32_t die_num);

/**
 * @brief Function to unmap ATU for DDRSS Config Space
 * 
 * @param die_num - die number
 */
void ddrss_atu_unmap_cfg_space(uint32_t die_num);

uintptr_t ddrss_atu_map_media_addr(uint64_t p_addr_8K_aligned);
void ddrss_atu_unmap_media_addr(uint64_t p_addr_8K_aligned);