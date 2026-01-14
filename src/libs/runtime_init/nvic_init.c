//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file nvic_init.c
 * Initialize the NVIC
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <nvic.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(nvic, FPFW_INIT_DEPENDENCIES("mpu"))
{
    // Initialize the NVIC and reallocate the Vector Table
    nvic_init(true);

    // Enable the Usage, Bus and Memory faults which are disabled by default
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk;

    // Enable DebugMonitor_IRQn for bugcheck.
    DCB->DEMCR |= DCB_DEMCR_MON_EN_Msk;

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
