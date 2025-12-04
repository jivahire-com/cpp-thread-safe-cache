//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_svc_mcp_init.c
 * Initializes the MCP telemetry service.
 */

/*------------- Includes -----------------*/
#include <fpfw_cfg_mgr.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <pwr_tlm_cli_service.h>
#include <telemetry_service.h>

/*-- Symbolic Constant Macros (defines) --*/

/**
 * @brief Note: If changing this value review ddr_manager_i.h
 *  max samples per package is used to size the memory pool for instantaneous packages.
 *
 */
#define MAX_INST_SAMPLES_PER_PACKAGE (20)

#define MILLISECONDS_PER_SECOND (1000)
#define SECONDS_PER_MINUTE      (60)
#define MINUTES_PER_HOUR        (60)
#define HOURS_PER_DAY           (24)

#define MILLISECONDS_PER_DAY (MILLISECONDS_PER_SECOND * SECONDS_PER_MINUTE * MINUTES_PER_HOUR * HOURS_PER_DAY)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(pwr_tlm_svc_mcp,
                    FPFW_INIT_DEPENDENCIES("sensor_fifo", "mts_svc", "hw_ver", "atu_svc", "gtimer", "hw_sem", "core_info", "tlm_fuses", "pldm", "cfg_mgr"))
{
    power_tlm_knobs_t pwr_tlm_knobs = config_get_pwr_tlm_knobs();

    uint32_t pwr_pkg_period_ms = (120 * MILLISECONDS_PER_SECOND);
    if (pwr_tlm_knobs.prod_pkg_period == PWR_TLM_PROD_PKG_PERIOD_30_SEC)
    {
        pwr_pkg_period_ms = 30 * MILLISECONDS_PER_SECOND;
    }
    else if (pwr_tlm_knobs.prod_pkg_period == PWR_TLM_PROD_PKG_PERIOD_1_MIN)
    {
        pwr_pkg_period_ms = SECONDS_PER_MINUTE * MILLISECONDS_PER_SECOND;
    }

    uint32_t inst_pkg_sample_period_ms = pwr_tlm_knobs.inst_sample_period * 5; // resolution of knob is 5ms

    uint16_t inst_samples_per_pkg = pwr_tlm_knobs.inst_samples_per_pkg;
    if (inst_samples_per_pkg > MAX_INST_SAMPLES_PER_PACKAGE)
    {
        inst_samples_per_pkg = MAX_INST_SAMPLES_PER_PACKAGE;
    }

    uint32_t _24_hr_pkg_sample_period_ms = MILLISECONDS_PER_DAY;
    if (pwr_tlm_knobs._24hr_sample_period == PWR_TLM_24HR_PKG_PERIOD_60_MIN)
    {
        _24_hr_pkg_sample_period_ms = 60 * SECONDS_PER_MINUTE * MILLISECONDS_PER_SECOND;
    }
    else if (pwr_tlm_knobs._24hr_sample_period == PWR_TLM_24HR_PKG_PERIOD_30_MIN)
    {
        _24_hr_pkg_sample_period_ms = 30 * SECONDS_PER_MINUTE * MILLISECONDS_PER_SECOND;
    }

    power_tlm_mpam_mcp_knobs_t mpam_knobs = config_get_pwr_tlm_mpam_mcp_knobs();

    telemetry_service_init(idsw_get_die_id(),
                           pwr_pkg_period_ms,
                           inst_pkg_sample_period_ms,
                           inst_samples_per_pkg,
                           _24_hr_pkg_sample_period_ms,
                           mpam_knobs.fixed_pwr_mW,
                           mpam_knobs.mem_pwr_primary_enable,
                           pwr_tlm_knobs.all_zero_filtering_enable,
                           idhw_is_single_die_boot_en());

    pwr_tlm_cli_svc_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}