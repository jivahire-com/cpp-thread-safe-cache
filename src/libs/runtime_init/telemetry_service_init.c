//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_service_init.c
 * Initializes the telemetry service.
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <telemetry_service.h>

/*-- Symbolic Constant Macros (defines) --*/
// note, eventually these will be replaced with a config knob
#define POWER_TLM_REPORT_PERIOD_MS (60 * 1000)
#define INST_TLM_REPORT_PERIOD_MS (120 * 1000)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/


FPFW_INIT_COMPONENT(tlm_svc, FPFW_INIT_DEPENDENCIES("sensor_fifo", "hw_ver", "atu_svc"))
{
    telemetry_service_init(idsw_get_die_id(), POWER_TLM_REPORT_PERIOD_MS, INST_TLM_REPORT_PERIOD_MS);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}