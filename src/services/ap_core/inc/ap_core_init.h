// Copyright Microsoft. All rights reserved.

/**
 * @file ap_core_init.h
 * This file contains the interface needed for APcore service init
 */

#pragma once

/*----------- Nested includes ------------*/

#include <DfwkCommon.h>
#include <corebits.h>
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct {
    DFWK_DEVICE_HEADER header;
    DFWK_QUEUE default_queue;
} ap_core_service_t, *pap_core_service_t;

typedef struct  {
    DFWK_INTERFACE_HEADER header;
    pap_core_service_t p_device;
} ap_core_interface_t, *pap_core_interface_t;

/**
 * @brief APcore service config
 */
typedef struct _ap_core_service_config_t
{
    uint64_t boot_core_rvbaraddr;           // boot core rvbar address
    uintptr_t cluster_pex_base;             // cluster pex base address
    uint32_t cluster_stride;                // number of bytes between each cluster
    const corebits_t* platform_cores_in_die; // platform cores
    unsigned int platform_die_core_count;    // platform core count
} ap_core_service_config_t, *pap_core_service_config_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

void ap_core_interface_init(pap_core_service_t p_device, pap_core_interface_t p_interface);
void ap_core_init(pap_core_service_t p_device, PDFWK_SCHEDULE p_schedule, const ap_core_service_config_t* p_config);


