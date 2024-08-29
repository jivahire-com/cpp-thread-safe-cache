// Copyright Microsoft. All rights reserved.

/**
 * @file ap_core.h
 * This file contains the interface to the APcore service
 */

#pragma once

/*----------- Nested includes ------------*/

#include <DfwkCommon.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/**
 *  Synchonous and Asynchronous Interface Request IDs
 */
enum APCORE_REQUEST_IDS
{
    APCORE_SET_RVBARADDR_ASYNC = 0x0,
    APCORE_CORE_POWER_ON_ASYNC,
    APCORE_CORE_POWER_OFF_ASYNC,
};

typedef struct _ap_core_asynchronous_request_t
{
    DFWK_ASYNC_REQUEST_HEADER header;
    union
    {
        uint32_t core_id;
        uint64_t rvbaraddr;
    } data;
} ap_core_asynchronous_request_t, *pap_core_asynchronous_request_t;

/*-- Declarations (Statics and globals) --*/
#define DEFAULT_POWER_TRANSITION_TIMEOUT_MS 10*1000

/*--------- Function Prototypes ----------*/

/**
 *  Function to send an asynchronous request to set rvbaraddr of all cores (local and remote die)
 *
 *  @param p_interface The interface to the APCore service
 *  @param p_request A DFWK request with enough space for a ap_core_asynchronous_request_t
 *  @param rvbaraddr The rvbaraddr to set for all cores
 *  @param completion_routine A completion routine to call when the operation has completed
 *  @param p_completion_context A context that is supplied when the completion routine is called.
 */
void ap_core_set_rvbaraddr(PDFWK_INTERFACE_HEADER p_interface,
                          pap_core_asynchronous_request_t p_request,
                          uint64_t rvbaraddr,
                          DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                          void* p_completion_context);

/**
 *  Function to send an asynchronous request to power on a core
 *
 *  @param p_interface The interface to the APCore service
 *  @param p_request A DFWK request with enough space for a ap_core_asynchronous_request_t
 *  @param core_id The core to power on
 *  @param completion_routine A completion routine to call when the operation has completed
 *  @param p_completion_context A context that is supplied when the completion routine is called.
 */
void ap_core_core_power_on(PDFWK_INTERFACE_HEADER p_interface,
                          pap_core_asynchronous_request_t p_request,
                          uint32_t core_id,
                          DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                          void* p_completion_context);

/**
 * @brief Function to send an asynchronous request to power off a core
 *
 * @param p_interface The interface to the APCore service
 * @param p_request A DFWK request with enough space for a ap_core_asynchronous_request_t
 * @param core_id The core to power off
 * @param completion_routine A completion routine to call when the operation has completed
 * @param p_completion_context A context that is supplied when the completion routine is called.
 */
void ap_core_core_power_off(PDFWK_INTERFACE_HEADER p_interface,
                           pap_core_asynchronous_request_t p_request,
                           uint32_t core_id,
                           DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                           void* p_completion_context);
