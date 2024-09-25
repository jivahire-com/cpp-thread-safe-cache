//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_stub_i.h
 * Header containing internal definitions for power service runtime configuration structures (static, knobs, fuse, etc)
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
//#define POWER_FUSE_PLAT_STUB_WAR        1

/*--------- Function Prototypes ----------*/

/**
 * @brief Informs if the current platform supports hardware fuses. This is supposed to be platform specific function
 *
 *
 * @return tue or flase
 *
 */
bool power_fuses_is_power_hw_supported();

/**
 * @brief Reads the fuse value at specified offset and size and stores it to target_addr
 *
 * @param[in] fuse_bit_offset   - relative offset of the fuse bits in the fuse
 * @param[in] fuse_bit_size     - Size of the fuse bits to be copied          
 * @param[out] target_addr      - Pointer to address where the fuse data should be copied
 * 
 * @return FPFW_STATUS_SUCCESS or FPFW_STATUS_INVALID_ARGS
 */
int32_t platform_read_fuse(const uint32_t * target_addr, const uint32_t fuse_bit_offset, const uint32_t fuse_bit_size);

// TODO: replace these alternative functions with update to silibs dvfs lib
//       https://dev.azure.com/AzureCSI/Dev/_workitems/edit/2058887
void dvfs_ns_set_cppc_desired2(const uintptr_t cluster_pex_base_addr, uint8_t cppc_desired, uint8_t cppc_base_perf, uint8_t throttle_pri, uint8_t boost_pri);
int dvfs_ns_get_cppc_desired2(const uintptr_t cluster_pex_base_addr, uint8_t *cppc_desired, uint8_t* cppc_base_perf, uint8_t *throttle_pri, uint8_t *boost_pri);