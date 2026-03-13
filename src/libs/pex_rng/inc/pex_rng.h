//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file pex_rng.h
 *  Calls the Crypto RNG Library for KNG SOC 
 */

#pragma once

/*--------------- Includes ---------------*/
#include <DfwkDriver.h> 
#include <stdint.h>  
#include <corebits.h>
#include <utils.h>

/*--------------- Typedefs ---------------*/

typedef struct _pex_rng_config_t
{
    uintptr_t cluster_pex_base;              // cluster pex base address
    uint32_t cluster_stride;                 // number of bytes between each cluster
    const corebits_t* platform_cores_in_die; // platform cores
    unsigned int core_count;                 // platform core count
} pex_rng_config_t, *ppex_rng_config_t;

typedef struct {
    DFWK_ASYNC_REQUEST_HEADER Header;
} pex_rng_request_t, *pex_rng_request_ptr_t;

enum pex_request_type
{
    PEX_SEND_CPER = 1, // Request type to send CPER
};

typedef struct {
    DFWK_DEVICE_HEADER header;
    pex_rng_config_t config;
    DFWK_QUEUE default_queue;
} pex_rng_device_t, *pex_rng_device_ptr_t;

typedef struct {
    DFWK_INTERFACE_HEADER header;
    pex_rng_device_t* pex_device;
} pex_rng_interface_t, *pex_rng_interface_ptr_t;

/*--------------- Function Prototypes ---------------*/
/**
 * Initialize/Enable the RNG instances in each core in normal mode by setting the control reg enable field to 1
 *  @param rng_config config structure is used to get the base address of the RNG instance and iterate through clusters
 *  @param is_warm_start if true, skip RNG hardware enable (already configured) and only initialize DFWK structures
 */
void init_pex_rng(pex_rng_config_t* rng_config, bool is_warm_start);
void reset_pex_rng(uintptr_t ap_rng_base);

void schedule_pex_error_handling_dfwk(void* context);

void register_pex_error_domain(pex_rng_config_t* pex_config);
