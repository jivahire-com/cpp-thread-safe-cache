//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_service_init.c
 * Initializes the telemetry service.
 */

/*------------- Includes -----------------*/
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <telemetry_cli_service.h>
#include <telemetry_service.h>

/*-- Symbolic Constant Macros (defines) --*/

/**
 * @brief Note: If changing this value review ddr_manager_i.h
 *  max samples per package is used to size the memory pool for instantaneous packages.
 *
 */
#define MAX_INST_SAMPLES_PER_PACKAGE (20)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(tlm_svc, FPFW_INIT_DEPENDENCIES("sensor_fifo", "mts_svc", "hw_ver", "atu_svc", "gtimer"))
{
    power_tlm_knobs_t pwr_tlm_knobs = config_get_pwr_tlm_knobs();

    uint32_t pwr_pkg_period_ms = (120 * 1000);
    if (pwr_tlm_knobs.prod_pkg_period == PWR_TLM_PROD_PKG_PERIOD_30_SEC)
    {
        pwr_pkg_period_ms = 30 * 1000;
    }
    else if (pwr_tlm_knobs.prod_pkg_period == PWR_TLM_PROD_PKG_PERIOD_1_MIN)
    {
        pwr_pkg_period_ms = 60 * 1000;
    }

    uint32_t inst_pkg_sample_period_ms = pwr_tlm_knobs.inst_sample_period * 5; // resolution of knob is 5ms

    uint16_t inst_samples_per_pkg = pwr_tlm_knobs.inst_samples_per_pkg;
    if (inst_samples_per_pkg > MAX_INST_SAMPLES_PER_PACKAGE)
    {
        inst_samples_per_pkg = MAX_INST_SAMPLES_PER_PACKAGE;
    }

    telemetry_service_init(idsw_get_die_id(), pwr_pkg_period_ms, inst_pkg_sample_period_ms, inst_samples_per_pkg);

    telemetry_cli_svc_initialize();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}