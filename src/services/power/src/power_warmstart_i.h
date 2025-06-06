//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_warmstart_i.h
 * Header containing internal definitions for power service warm start.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <power_runconfig_i.h>

/*-- Symbolic Constant Macros (defines) --*/
#define POWER_WARMSTART_VER1        0x00000001
#define POWER_WARMSTART_CURRENT_VER POWER_WARMSTART_VER1

// separate set of defines for things that are not based on HW and could possibly change over time
#define WS_VFT_COUNT 7 /* the number of VFTs we're expecting from fuses */

// go ahead and cause build errors if these separate defines above don't match what we use elsewhere
// (a build error would mean that we need to version the WS define and the structures it's used in)
static_assert(WS_VFT_COUNT == VFT_CURVESET_COUNT, "WS_VFT_COUNT != VFT_CURVESET_COUNT");

/*-------------- Typedefs ----------------*/
/**
 * @brief Struct for warmstart VFT point
 */
typedef struct
{
    uint16_t ldo_dac_in[VFT_CURVE_COUNT_PER_CURVESET];
    uint16_t freq_Mhz;   // frequency in megahertz
    uint16_t voltage_mv; // voltage in mv
} power_ws_vft_point_t;

/**
 * @brief Struct for warmstart VFT
 */
typedef struct
{
    corebits_t cores;                     // cores which were assigned to this table
    power_ws_vft_point_t vf[NUM_PSTATES]; // vf points for VF table
    uint8_t min_plimit;                   // minimum possible plimit based on curve pairs
} power_ws_vft_t;

/**
 * @brief Struct for warmstart PE core data
 * This is the data that is saved in the warmstart data
 */
typedef struct
{
    uint32_t version;           // version of warmstart data
    corebits_t valid_cores;     // fuse data for valid cores
    power_ws_vft_t vfts[VFT_CURVESET_COUNT];

/* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491054/
   Add temperature detail to these curve/curveset structures as needed for ITD
 */
} power_ws_fuse_t, *ppower_ws_fuse_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief Recover the runconfig from the stored warm start data structure
 * 
 * @param runconfig[in] Power runconfig structure to recover
 */
void power_ws_recover_fuse_init(power_runconfig_t *runconfig);

/**
 * @brief Initialize the warm start data and store in the warm start data structure
 * 
 * @param runconfig[in] Power runconfig structure to save
 */
void power_ws_save_fuse_init(const power_runconfig_t *runconfig);
