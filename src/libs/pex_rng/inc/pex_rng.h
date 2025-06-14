//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file pex_rng.h
 *  Calls the Crypto RNG Library for KNG SOC 
 */

#pragma once

/*--------------- Includes ---------------*/
#include <stdint.h>  
#include <corebits.h>

/*--------------- Typedefs ---------------*/

typedef struct _pex_rng_config_t
{
    uintptr_t cluster_pex_base;              // cluster pex base address
    uint32_t cluster_stride;                 // number of bytes between each cluster
    const corebits_t* platform_cores_in_die; // platform cores
    unsigned int core_count;                 // platform core count
} pex_rng_config_t, *ppex_rng_config_t;


/*--------------- Function Prototypes ---------------*/
/**
 * Initialize/Enable the RNG instances in each core in normal mode by setting the control reg enable field to 1
 *  @param rng_config config structure is used to get the base address of the RNG instance and iterate through clusters
 */
void init_pex_rng(pex_rng_config_t* rng_config);
void reset_pex_rng(uintptr_t ap_rng_base);
