//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file etc_etd_init.c
 * Instantiates Event Tracing Collector for the MCP and SCP cores
 */

/*------------- Includes -----------------*/

#include <etc_etd_svc.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW...
#include <stddef.h>    // for NULL

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(etd, FPFW_INIT_DEPENDENCIES("std_io", "debug_print"))
{
    etd_service_context_t* context;
    etd_svc_init();
    context = get_etd_service_context();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, context};
}

FPFW_INIT_COMPONENT(etc, FPFW_INIT_DEPENDENCIES("etd"))
{
    etc_service_context_t* context;
    etc_svc_init();
    context = get_etc_service_context();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, context};
}
