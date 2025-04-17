//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_runconfig.c
 * Implements runtime configuration portion of power service
 */

/*------------- Includes -----------------*/

#include "power_runconfig.h"

#include "fpfw_status.h"
#include "power_hw_int_i.h"
#include "power_i.h"
#include "power_runconfig_i.h"

#include <bug_check.h> // for BUG_CHECK
#include <corebits.h>  // for corebits_is_bit_set, corebits_is_clear
#include <dvfs.h>
#include <dvfs_struct.h> // for dvfs_vft_t, NUM_PSTATES, dvfs_vf_fuse...
#include <idsw_kng.h>
#include <silibs_common.h> // for MAX, MIN
#include <stdbool.h>
#include <string.h> // for memset

/*-- Symbolic Constant Macros (defines) --*/
/* Setting all pwr set commands to a generic fake dummy_set_function. To be updated (ADO: 1887411) */
#define DUMMY_SET_CAP_FUNCTION            dummy_set_function
#define DUMMY_SET_DESIRED_PSTATE_FUNCTION dummy_set_function
#define DUMMY_SET_PLIMIT_FUNCTION         dummy_set_function
#define DUMMY_SET_LOOP_DISABLES_FUNCTION  dummy_set_function
#define DUMMY_SET_RACK_LIMIT_FUNCTION     dummy_set_function
#define DUMMY_SET_MINUPDATE_FUNCTION      dummy_set_function
#define DUMMY_SET_NOMINAL_FUNCTION        dummy_set_function
#define DUMMY_SET_FORCED_FUNCTION         dummy_set_function

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void dummy_set_function(char* p_string, void* p_set_data);

/*-- Declarations (Statics and globals) --*/

static power_runconfig_t power_runconfig = {};
static power_adclk_tel_t adclk_tel = {};
static TX_MUTEX adclk_tel_mutex;

power_runconfig_read_dictionary_element_t power_runconfig_read_dictionary[] = {
    {POWER_IF_CMD_GET_RUNCONFIG_FUSES, &power_runconfig.fuses},
    {POWER_IF_CMD_GET_RUNCONFIG_KNOBS, &power_runconfig.knobs},
};

// clang-format off
/* Setting all pwr set commands to fake dummy set functions. To be updated (ADO: 1887411) */
power_runconfig_write_dictionary_element_t power_runconfig_set_dictionary[] = {
    {POWER_IF_CMD_SET_CAP,              DUMMY_SET_CAP_FUNCTION              , "power cap"           },
    {POWER_IF_CMD_SET_DESIRED_PSTATE,   DUMMY_SET_DESIRED_PSTATE_FUNCTION   , "desired pstate"      },
    {POWER_IF_CMD_SET_PLIMIT,           DUMMY_SET_PLIMIT_FUNCTION           , "power limit"         },
    {POWER_IF_CMD_SET_LOOP_DISABLES,    DUMMY_SET_LOOP_DISABLES_FUNCTION    , "loop disables"       },
    {POWER_IF_CMD_SET_RACK_LIMIT,       DUMMY_SET_RACK_LIMIT_FUNCTION       , "rack power limit"    },
    {POWER_IF_CMD_SET_MINUPDATE,        DUMMY_SET_MINUPDATE_FUNCTION        , "min update interval" },
    {POWER_IF_CMD_SET_NOMINAL,          DUMMY_SET_NOMINAL_FUNCTION          , "nominal pstate"      },
    {POWER_IF_CMD_SET_FORCED,           DUMMY_SET_FORCED_FUNCTION           , "forced pstate ldodacin" },
    {POWER_IF_CMD_SET_PSTATE_FREQ,     DUMMY_SET_FORCED_FUNCTION           , "pstate table"        },

};
// clang-format on

const uint32_t length_power_runconfig_read_dictionary =
    sizeof(power_runconfig_read_dictionary) / sizeof(power_runconfig_read_dictionary_element_t);

const uint32_t length_power_runconfig_set_dictionary =
    sizeof(power_runconfig_set_dictionary) / sizeof(power_runconfig_write_dictionary_element_t);

/*------------- Functions ----------------*/

power_runconfig_t* power_runconfig_get()
{
    return &power_runconfig;
}

/*------------- Functions ----------------*/
void power_init_adclk_tel_mutex()
{
    BUG_ASSERT(tx_mutex_create(&adclk_tel_mutex, "adclk_tel_mutex", TX_INHERIT) == TX_SUCCESS);
}

void power_adclk_tel_mutex_lock()
{
    tx_mutex_get(&adclk_tel_mutex, TX_WAIT_FOREVER);
}

void power_adclk_tel_mutex_unlock()
{
    tx_mutex_put(&adclk_tel_mutex);
}

void power_get_adclk_telem(power_adclk_tel_t* dest_adclk_tel)
{
    FPFW_RUNTIME_ASSERT(dest_adclk_tel != 0);
    power_adclk_tel_mutex_lock();
    memcpy(dest_adclk_tel, &adclk_tel, sizeof(power_adclk_tel_t));
    power_adclk_tel_mutex_unlock();
}

power_adclk_tel_t* power_get_adclk_telem_ptr()
{
    return &adclk_tel;
}

void power_reset_adclk_telem()
{
    power_adclk_tel_mutex_lock();
    memset(&adclk_tel, 0, sizeof(power_adclk_tel_t));
    power_adclk_tel_mutex_unlock();
}

void* power_runconfig_get_element(power_if_cmd_t id)
{
    for (uint32_t index = 0; index < length_power_runconfig_read_dictionary; index++)
    {
        /* Return the pointer to the element referenced by the power runconfig element ID */
        if (id == power_runconfig_read_dictionary[index].id)
        {
            return power_runconfig_read_dictionary[index].p_runconfig_element;
        }
    }

    return NULL;
}

void power_runconfig_set_element(power_if_cmd_t id, void* p_set_data)
{
    for (uint32_t index = 0; index < length_power_runconfig_set_dictionary; index++)
    {
        /* Return the pointer to the element referenced by the power runconfig element ID */
        if (id == power_runconfig_set_dictionary[index].id)
        {
            power_runconfig_set_dictionary[index].p_function(power_runconfig_set_dictionary[index].p_string, p_set_data);
        }
    }
}

/* dummy_set_function to be removed (ADO: 1887411) */
void dummy_set_function(char* p_string, void* p_set_data)
{
    FPFW_UNUSED(p_set_data);

    printf("Setting value of %s in placeholder set function\n", p_string);
}

/* function to return the min_plimit supported by all cores */
static uint8_t get_global_min_plimit()
{
    uint8_t min_plimit = 0;
    for (unsigned vf_idx = 0; vf_idx < VFT_CURVESET_COUNT; ++vf_idx)
    {
        // skip if no cores assigned
        if (corebits_is_clear(&power_runconfig.derived.vfts[vf_idx].assigned_cores))
        {
            continue;
        }
        min_plimit = MAX(min_plimit, power_runconfig.derived.vfts[vf_idx].min_plimit);
    }
    return min_plimit;
}

static void runconfig_generate_derived_vfts()
{

    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();

    // setup assigned vft/core mappings
    for (unsigned core_idx = 0; core_idx < NUM_AP_CORES_PER_DIE; ++core_idx)
    {
        unsigned curve_assignment = 0;
        // only set core assignment for enabled cores
        const bool core_disabled = !corebits_is_bit_set(&power_runconfig.fuses.valid_cores, core_idx);
        if (core_disabled)
        {
            continue;
        }
        int status = power_fuses_get_curve_assignment(core_idx, (uint32_t*)&curve_assignment);
        if ((curve_assignment >= VFT_CURVESET_COUNT) || (status != FPFW_STATUS_SUCCESS))
        {
            BUG_CHECK(KNG_SC_FUSE_CURVE_ASSIGNMENT, core_idx, curve_assignment);
        }
        corebits_set_bit(&power_runconfig.derived.vfts[curve_assignment].assigned_cores, core_idx);
        power_runconfig.derived.assigned_vft[core_idx] = curve_assignment;
    }

    // generate full VFT for DVFS use as well as v/f pairs for power loop
    for (unsigned vf_idx = 0; vf_idx < VFT_CURVESET_COUNT; ++vf_idx)
    {
        uint8_t min_plimit = 0;
        uint8_t last_valid_curve = 0;
        for (unsigned crv_idx = 0; crv_idx < VFT_CURVE_COUNT_PER_CURVESET; ++crv_idx)
        {
            // TODO: fix curve structure for ITD - shouldn't have 4 curves and 4 vmat_info (https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1491054/)
            int status =
                dvfs_vft_from_fuse_data_per_itd(&power_runconfig.fuses.vf.curveset[vf_idx].curve[crv_idx],
                                                &power_runconfig.fuses.memasst,
                                                &min_plimit,
                                                &power_runconfig.dvfs_vft.curveset[vf_idx].vmat_info[crv_idx]);

            if (DVFS_SUCCESS != status)
            {
                BUG_CHECK(KNG_SC_FUSE_GEN_VFT, vf_idx, status);
            }
            else
            {
                last_valid_curve = crv_idx;
            }

            if (min_plimit == NUM_PSTATES)
            {
                dvfs_vft_from_fuse_data_per_itd(&power_runconfig.fuses.vf.curveset[vf_idx].curve[last_valid_curve],
                                                &power_runconfig.fuses.memasst,
                                                &min_plimit,
                                                &power_runconfig.dvfs_vft.curveset[vf_idx].vmat_info[last_valid_curve]);
            }

            // track min_plimit to ensure all curves have same
            if (crv_idx > 0)
            {

                BUG_ASSERT_PARAM(min_plimit == power_runconfig.derived.vfts[vf_idx].min_plimit,
                                 min_plimit,
                                 power_runconfig.derived.vfts[vf_idx].min_plimit);
            }
            else
            {
                // store the min_plimit for this curve set
                power_runconfig.derived.vfts[vf_idx].min_plimit = min_plimit;
            }
        }

        // special case for VF curve 0; if no fused curve, use default
        if ((vf_idx == 0) && (power_runconfig.derived.vfts[vf_idx].min_plimit == NUM_PSTATES))
        {
            // put default DVFS curve in slot 0
            const dvfs_vft_t default_vft = DVFS_VFT_DEFAULT_CONFIG;
            for (unsigned crv_idx = 0; crv_idx < VFT_CURVESET_COUNT; ++crv_idx)
            {
                power_runconfig.dvfs_vft.curveset[crv_idx] = default_vft;
            }
            POWER_LOG_WARN("Using default VF curve for curve 0");
            power_runconfig.derived.vfts[vf_idx].min_plimit = 0;
        }
        // convert the dvfs vft to voltage/freq pairs for use in power loop
        for (unsigned pstate_idx = power_runconfig.derived.vfts[vf_idx].min_plimit; pstate_idx < NUM_PSTATES; ++pstate_idx)
        {
            // we want to find the max voltage for any curve in curveset, since this will be what we're
            // required to be able to support out of the ldo when this plimit is selected
            uint16_t found_voltage_mv = 0;
            for (unsigned crv_idx = 0; crv_idx < VFT_CURVE_COUNT_PER_CURVESET; ++crv_idx)
            {
                uint16_t voltage_mv;
                int status = dvfs_mvolt_from_ldo_dac(
                    power_runconfig.dvfs_vft.curveset[vf_idx].vmat_info[crv_idx].ldo_dac_in[pstate_idx],
                    &power_runconfig.fuses.ldodac_to_volt,
                    &voltage_mv);
                if (DVFS_SUCCESS != status && platform_id != PLATFORM_SVP_SIM)
                {
                    BUG_CHECK(KNG_SC_FUSE_LDO_MVOLT_CONV, (pstate_idx << 16) | vf_idx, status);
                }
                found_voltage_mv = MAX(found_voltage_mv, voltage_mv);
            }
            power_runconfig.derived.vfts[vf_idx].vf[pstate_idx].voltage_mv = found_voltage_mv;
            power_runconfig.derived.vfts[vf_idx].vf[pstate_idx].freq_Mhz = dvfs_get_freq_from_plimit(pstate_idx);
        }
    }
}

static void runconfig_generate_derived_tdp()
{
    // establish TDP-based runtime defaults

    // get fused nominal pstate from tdp freq
    const uint8_t fused_pstate = power_hw_pstate_from_freq(power_runconfig.fuses.tdp_config.freq_MHz);
    // determine if knob override or fuse is used for nominal
    power_runconfig.derived.pnominal =
        (power_runconfig.knobs.nominal_pstate == 0) ? fused_pstate : power_runconfig.knobs.nominal_pstate;
    // enforce bounds required in either code/HW
    power_runconfig.derived.pnominal = MAX(power_runconfig.derived.pnominal, NOMINAL_PSTATE_MIN);
    power_runconfig.derived.pnominal = MIN(power_runconfig.derived.pnominal, NOMINAL_PSTATE_MAX);

    // check that all cores support nominal
    if (power_runconfig.derived.pnominal < get_global_min_plimit())
    {
        BUG_CHECK(KNG_SC_FUSE_INVALID_NOMINAL, power_runconfig.derived.pnominal, get_global_min_plimit());
    }

    // determine if knob override or fused value used for thermal watts limit
    power_runconfig.derived.soc_maximum_thermal_watts_limit =
        (power_runconfig.knobs.soc_maximum_thermal_watts_limit == 0)
            ? power_runconfig.fuses.tdp_config.power_A
            : power_runconfig.knobs.soc_maximum_thermal_watts_limit;
}

void power_runconfig_init(const power_service_config_t* p_config)
{
    // ensure the structure is zeroed - this is important to be done this way for the derived data during test
    memset(&power_runconfig, 0, sizeof(power_runconfig_t));

    // reset the adclk telemetry counter during initialization.
    power_init_adclk_tel_mutex();
    power_reset_adclk_telem();

    // store the config pointer
    power_runconfig.p_sconfig = p_config;

    // read power knobs
    power_knobs_read(&power_runconfig.knobs);

    // power fuse read
    power_fuses_read(&power_runconfig.fuses);

    // generate derived data
    runconfig_generate_derived_vfts();
    runconfig_generate_derived_tdp();

    // update valid cores to only include those that are present
    corebits_and(&power_runconfig.fuses.valid_cores, p_config->platform_cores_in_die);
}
