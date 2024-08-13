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

// Enum for pmin type used in hw interface function
typedef enum _power_pmin_type_t
{
    PM_PMIN_ALL = 0,
    PM_PMIN_IOTEMP,
    PM_PMIN_POWER_CAP,
} power_pmin_type_t;

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

/**
 * @brief Sets force PMIN bit to force HW to minimum (perf) pstate
 *
 * @param[in] type   - type of pmin to force
 *
 * @return none
 *
 */
void power_hw_force_pmin(power_pmin_type_t type);

/**
 * @brief Clears force PMIN bits to enable HW to leave minimum (perf) pstate
 *
 * @param[in] type   - type of pmin to force
 *
 * @return none
 *
 */
void power_hw_clear_force_pmin(power_pmin_type_t type);