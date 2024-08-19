//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_memory_map.h
 * DDR module - Static DDR reservations to be allocated for SCP and MCP
 */

#pragma once

/*----------- Nested includes ------------*/
#include <ddrss_lib.h>
#include <stdint.h>
#include "memory_map/ddrss_region_def.h"
#include "memory_map/ddrss_reserved_regions.h"

/*-- Symbolic Constant Macros (defines) --*/


/*----------- Typedefs -------------------*/

/*--------Funciton Prototypes-----------*/
int sort_reserved_regions(const ddrss_memory_region_t reservations[], uint32_t num_rsvd, ddrss_memory_region_t out_sorted[]);
int check_reservation_order(const ddrss_memory_region_t reservations[]);
int ddrmap_add_reservations(const ddrss_memory_region_t in_mmap[],
                            const ddrss_memory_region_t reservations[],
                            ddrss_memory_region_t out_mmap[]);
int ddrmap_get_last_idx(const ddrss_memory_region_t ddr_mmap[]);
void ddrss_get_memory_map(const ddrss_sys_mem_region_t **mem_regions);
int get_mmap_size_bytes(ddrss_memory_region_t appended_mmap_64b[], size_t *num_bytes);
/*
 * Declared in include/ddrss_lib.h
 * ---------------------------------------------------------------------------------------------------
 *   |----> ddrss_sys_mem_region_t
 *          |----> ddrss_memory_region_t[]
 *                 |------> uint64_t start_address
 *                 |------> uint64_t end_address
 *                 |------> memory_region_attributes_t  attr
 *                          |--------> uint32_t secure: 1
 *                          |--------> uint32_t non_secure: 1
 *                          |--------> uint32_t root: 1
 *                          |--------> uint32_t realm: 1
 *                          |--------> uint32_t available_sysmem: 1
 *                                     (union) as_uint32
 *          |----> uint32_t num_regions
 *----------------------------------------------------------------------------------------------------
 */

/* DDRSS_MAX_MEM_REGIONS is defined in ddrss_lib.h
 *   +--------------------------------+   +--------------------------------+
 *   |           Region 1             |   |           Region 1             |
 *   +--------------------------------+   +--------------------------------+
 *   +----------+----------+----------+   +--------------+-----------------+
 *   | Region 1 | Inserted | Region 1 |   | Inserted     |     Region 1    |
 *   |  lower   |  Region  |   upper  |   |  Region      |     remainder   |
 *   +----------+----------+----------+   +--------------+-----------------+
 */
#define MAX_MEMORY_REGIONS       DDRSS_MAX_MEM_REGIONS + ARRAY_OF_RSVD_REGIONS_COUNT

