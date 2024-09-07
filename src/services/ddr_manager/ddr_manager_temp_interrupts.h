//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_temp_interrupts.h
 * DDR Manager Temperature Interrupt APIs
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
// Per DRAM temperature and min refersh rate request from DIMM
    //   1 - 1x Refresh Rate, <80C nominal.
    //   2 - 1x Refresh Rate, 80-85C nominal.
    //   3 - 2x Refresh Rate, 85-90C nominal.
    //   4 - 2x Refresh Rate, 90-95C nominal.
    //   5 - 2x Refresh Rate, >95C nominal.
    // Other values are invalid.
typedef enum {
    DDR_MR4_TEMP_INVALID = 0,
    DDR_MR4_TEMP_LT80 = 1,
    DDR_MR4_TEMP_80_85 = 2,
    DDR_MR4_TEMP_85_90 = 3,
    DDR_MR4_TEMP_90_95 = 4,
    DDR_MR4_TEMP_GT95 = 5,
} ddr_mr4_temp_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Register the DDR temperature interrupt callbacks
 */
void ddr_manager_temp_interrupts_init();

/**
 * @brief DDR critical temperature interrupt callback
 * @param mc Memory controller index
 */
void ddr_manager_temp_interrupts_critical_cb(uint32_t mc);

/**
 * @brief DDR temperature changed interrupt callback
 * @param mc Memory controller index
 */
void ddr_manager_temp_interrupts_changed_cb(uint32_t mc);

/**
 * @brief Process temperature changed event
 * @param mc Memory controller index
 */
void ddr_manager_temp_interrupts_process_changed_event(uint32_t mc);