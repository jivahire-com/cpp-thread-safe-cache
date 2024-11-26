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
#include <telemetry_cli_service.h>
#include <telemetry_service.h>

/*-- Symbolic Constant Macros (defines) --*/
// note, eventually these will be replaced with a config knob

/**
 * @brief Power Package records are sampled and sent utilizing the same timer
 *
 */
#define POWER_TLM_PKG_PERIOD_MS (120 * 1000)

/**
 * @brief Instantaneous Package records are sampled utilizing this period.
 *
 */
#define INST_TLM_PKG_SAMPLE_PERIOD_MS (10)

/**
 * @brief Number of samples per package. In order to reduce host communication overhead, multiple
 * record samples are sent in a single package.
 *
 */
#define INST_TLM_SAMPLES_PER_PKG (10)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/


FPFW_INIT_COMPONENT(tlm_svc, FPFW_INIT_DEPENDENCIES("sensor_fifo", "hw_ver", "atu_svc"))
{
    telemetry_service_init(idsw_get_die_id(), POWER_TLM_PKG_PERIOD_MS, INST_TLM_PKG_SAMPLE_PERIOD_MS, INST_TLM_SAMPLES_PER_PKG);

    telemetry_cli_svc_initialize();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}