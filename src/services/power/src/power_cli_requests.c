//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_cli_requests.c
 * Implements the handlers for CLI requests
 */

/*------------- Includes -----------------*/
#include "power_hw_int_i.h"
#include "power_i.h" // for POWER_LOG_INFO, power_ap_soc_init
#include "power_loops_i.h"
#include "power_runconfig_i.h" // for power_runconfig_get_element, power...

#include <DfwkDriver.h> // for DfwkAsyncRequestComplete, DfwkInte...
#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <dvfs.h>       // for dvfs_get_cppc_from_pstate, dvfs_pll_g...
#include <power_dfwk.h> // for (anonymous), ppower_service_cli_re...
#include <stdbool.h>    // for false
#include <stddef.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void power_set_cap(uint16_t cap_value, ppower_service_cli_request_t p_request);

static void power_set_cap_callback(int result, uint16_t current, uint16_t previous);
static void power_set_minupdate(power_runconfig_t* p_runconfig, uint16_t minupdate_val);

/*-- Declarations (Statics and globals) --*/

static ppower_service_cli_request_t sp_cli_cap_request = NULL;

/*------------- Functions ----------------*/
// this is the callback function for setting a power cap
static void power_set_cap_callback(int result, uint16_t current, uint16_t previous)
{
    if (sp_cli_cap_request)
    {
        // put response values in structure

        sp_cli_cap_request->fetch_data.pwrset_response_val.pwr_icc_cap_result.current_cap = current;
        sp_cli_cap_request->fetch_data.pwrset_response_val.pwr_icc_cap_result.previous_cap = previous;
        sp_cli_cap_request->fetch_data.pwrset_response_val.pwr_icc_cap_result.result = result;

        // complete the request
        DfwkAsyncRequestComplete(&sp_cli_cap_request->header);
    }

    // clear after use
    sp_cli_cap_request = NULL;
}

// "pwr set cap" command
static void power_set_cap(uint16_t cap_value, ppower_service_cli_request_t p_request)
{

    // save off request so that we can complete it when the cap update is complete.
    sp_cli_cap_request = p_request;

    // send update to power_cap api
    power_cap_update(power_set_cap_callback, cap_value, true);
}

// "pwr set minupdate" command
static void power_set_minupdate(power_runconfig_t* p_runconfig, uint16_t minupdate_val)
{
    p_runconfig = power_runconfig_get(); //&power_runconfig;
    p_runconfig->knobs.minimum_plimit_updates = minupdate_val;
}

static void power_cli_requests_callback(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{

    FPFW_RUNTIME_ASSERT(p_request != NULL);
    FPFW_UNUSED(p_context);

    power_runconfig_t* p_runconfig = power_runconfig_get();
    const power_service_config_t* p_config = p_runconfig->p_sconfig;

    uint8_t core_count = p_config->platform_die_core_count;

    power_ctrl_loop_detail_t* s_ctrl_loop = get_s_ctrl_loop();

    bool all = 0;
    uint8_t core = 0;
    uint8_t desired = 0;
    uint8_t throttle = 0;
    uint16_t value = 0;
    uint8_t previous = 0;
    uint8_t forced_pstate = MAX_PLIMIT;

    switch (p_request->RequestType)
    {
    case CLI_COMMANDS_POWER_CONFIG: {
        ppower_service_cli_request_t p_cli_request = (ppower_service_cli_request_t)p_request;
        p_cli_request->fetch_data.p_requested_data = power_runconfig_get_element(p_cli_request->power_ext_if_cmd_id);
        DfwkAsyncRequestComplete(p_request);
        break;
    }

    case CLI_COMMANDS_POWER_SET: {
        ppower_service_cli_request_t p_cli_request = (ppower_service_cli_request_t)p_request;

        switch (p_cli_request->power_ext_if_cmd_id)
        {
        case POWER_IF_CMD_SET_CAP: {
            // use the data in the union
            power_set_cap(p_cli_request->pwrset_sub_command_args.cap_val, p_cli_request);
            // will complete request in callback
            break;
        }

        case POWER_IF_CMD_SET_DESIRED_PSTATE: {
            // use the data in the union
            all = p_cli_request->pwrset_sub_command_args.desiredparams.all;
            core = p_cli_request->pwrset_sub_command_args.desiredparams.core;
            throttle = p_cli_request->pwrset_sub_command_args.desiredparams.throttle;
            desired = dvfs_get_cppc_from_pstate(p_cli_request->pwrset_sub_command_args.desiredparams.state);

            do
            {
                const uintptr_t cluster_pex_base_addr =
                    (p_config->cluster_pex_base + (p_config->cluster_stride * core));
                if (core < core_count)
                { // NUM_AP_CORES_PER_DIE) {
                    dvfs_ns_set_cppc_desired(cluster_pex_base_addr, desired, throttle);

                    p_cli_request->fetch_data.pwrset_response_val.desiredparams.core = core;
                    p_cli_request->fetch_data.pwrset_response_val.desiredparams.cluster_pex_base_addr = cluster_pex_base_addr;
                    p_cli_request->fetch_data.pwrset_response_val.desiredparams.state = desired;
                    p_cli_request->fetch_data.pwrset_response_val.desiredparams.throttle = throttle;
                }
                else
                {
                    p_cli_request->fetch_data.pwrset_response_val.desiredparams.core = core;
                }
                ++core;

            } while (all && core < core_count); // power_context.core_count);

            // will complete request in callback
            DfwkAsyncRequestComplete(p_request);
            break;
        }

        case POWER_IF_CMD_SET_PLIMIT: {
            // use the data in the union
            all = p_cli_request->pwrset_sub_command_args.plimitparams.all;
            core = p_cli_request->pwrset_sub_command_args.plimitparams.core;
            desired = p_cli_request->pwrset_sub_command_args.plimitparams.state;

            do
            {
                const uintptr_t cluster_pex_base_addr =
                    (p_config->cluster_pex_base + (p_config->cluster_stride * core));

                if ((core < core_count) && corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core))
                {

                    dvfs_plimit plimit_as_uint32 = {
                        .vf_index = desired,
                        .power_cap = false,
                        .currthresh_1 = 0x66,
                        .currthresh_2 = 0xA7,
                        .currthresh_3 = 0xE6,
                    };

                    power_set_plimit(p_runconfig, core, plimit_as_uint32);

                    p_cli_request->fetch_data.pwrset_response_val.plimitparams.core = core;
                    p_cli_request->fetch_data.pwrset_response_val.plimitparams.cluster_pex_base_addr = cluster_pex_base_addr;
                    p_cli_request->fetch_data.pwrset_response_val.plimitparams.state = desired;
                }
                else
                {
                    p_cli_request->fetch_data.pwrset_response_val.plimitparams.core = core;
                }
                ++core;

            } while (all && core < core_count);

            // will complete request in callback
            DfwkAsyncRequestComplete(p_request);
            break;
        }

        case POWER_IF_CMD_SET_LOOP_DISABLES: {
            value = p_runconfig->knobs.loops_disable = p_cli_request->pwrset_sub_command_args.loopdis_bits;

            p_cli_request->fetch_data.pwrset_response_val.loopdis_bits = value;
            DfwkAsyncRequestComplete(p_request);
            break;
        }

        case POWER_IF_CMD_SET_MINUPDATE: {
            power_set_minupdate(p_runconfig, p_cli_request->pwrset_sub_command_args.minupdate_val);

            p_cli_request->fetch_data.pwrset_response_val.minupdate_val =
                p_cli_request->pwrset_sub_command_args.minupdate_val;

            DfwkAsyncRequestComplete(p_request);
            break;
        }

        case POWER_IF_CMD_SET_NOMINAL: {
            previous = p_runconfig->derived.pnominal;

            p_runconfig->derived.pnominal = p_cli_request->pwrset_sub_command_args.nominalparams.current_val;
            p_runconfig->derived.pnominal = MAX(p_runconfig->derived.pnominal, NOMINAL_PSTATE_MIN);
            p_runconfig->derived.pnominal = MIN(p_runconfig->derived.pnominal, NOMINAL_PSTATE_MAX);

            p_cli_request->fetch_data.pwrset_response_val.nominalparams.current_val = p_runconfig->derived.pnominal;
            p_cli_request->fetch_data.pwrset_response_val.nominalparams.previous_val = previous;

            DfwkAsyncRequestComplete(p_request);
            break;
        }

        case POWER_IF_CMD_SET_RACK_LIMIT: {
            power_hw_gpio_set_rack_limit_override(p_cli_request->pwrset_sub_command_args.racklimit != 0);
            s_ctrl_loop->rack_limit = (p_cli_request->pwrset_sub_command_args.racklimit != 0);
            p_cli_request->fetch_data.pwrset_response_val.racklimit = s_ctrl_loop->rack_limit;
            DfwkAsyncRequestComplete(p_request);
            break;
        }

        case POWER_IF_CMD_SET_FORCED: {
            p_cli_request->fetch_data.pwrset_response_val.forcedparams.pstate =
                p_cli_request->pwrset_sub_command_args.forcedparams.pstate;
            forced_pstate = p_cli_request->fetch_data.pwrset_response_val.forcedparams.pstate;
            p_cli_request->fetch_data.pwrset_response_val.forcedparams.ldodacin =
                p_cli_request->pwrset_sub_command_args.forcedparams.ldodacin;

            // Force pmin
            power_hw_force_pmin(PM_FW_PMIN_CONTROL);
            dvfs_vft_t forced_pstate_vft = {0}; // temp vft to use for any cores with forced pstates
            dvfs_config_t dvfs_cfg = DVFS_DEFAULT_CONFIG;

            // Set the ldodacin (Low dropout digital to analog converter input) for the forced pstate
            for (unsigned col_num = 0; col_num < NUM_DVFS_ITD_TEMPERATURE_LOOKUP_COLUMNS; ++col_num)
            {
                forced_pstate_vft.vmat_info[col_num].ldo_dac_in[forced_pstate] =
                    p_cli_request->fetch_data.pwrset_response_val.forcedparams.ldodacin;
            }
            dvfs_cfg.fuse_cfg.vft = &forced_pstate_vft;

            // Force the Pstate
            for (unsigned int core = 0; core < core_count; ++core)
            {
                const uintptr_t cluster_pex_base_addr =
                    (p_config->cluster_pex_base + (p_config->cluster_stride * core));
                const bool core_enabled = corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core);

                // Check if the core is valid and enabled
                if (corebits_is_bit_set(p_config->platform_cores_in_die, core) && core_enabled)
                {
                    printf("\nbase_addr = %d, forced_pstate = %d, ldodacin = %d, core = %d",
                           cluster_pex_base_addr,
                           forced_pstate,
                           forced_pstate_vft.vmat_info[0].ldo_dac_in[forced_pstate],
                           core);
                    setup_forced_pstate(cluster_pex_base_addr, &dvfs_cfg, &forced_pstate_vft, core, forced_pstate);
                }
            }

            // Set Plimit to P30 to avoid any possibility of throttling, even if the throttling is disabled.
            // P30 is programmed to be the desired frequency, and then set to be the Plimit.
            for (unsigned int core = 0; core < core_count; ++core)
            {
                const uintptr_t cluster_pex_base_addr =
                    (p_config->cluster_pex_base + (p_config->cluster_stride * core));
                const bool core_enabled = corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core);

                // Check if the core is valid and enabled
                if (corebits_is_bit_set(p_config->platform_cores_in_die, core) && core_enabled)
                {
                    dvfs_set_plimit(cluster_pex_base_addr, 30, false);
                }
            }
            // Clear the force pmin after setting the forced pstate
            power_hw_clear_force_pmin(PM_PMIN_ALL);

            DfwkAsyncRequestComplete(p_request);
            break;
        }

        case POWER_IF_CMD_SET_PSTATE_FREQ: {
            p_cli_request->fetch_data.pwrset_response_val.pstatefreq.pstate =
                p_cli_request->pwrset_sub_command_args.pstatefreq.pstate;
            p_cli_request->fetch_data.pwrset_response_val.pstatefreq.freq_ctrl =
                p_cli_request->pwrset_sub_command_args.pstatefreq.freq_ctrl;
            p_cli_request->fetch_data.pwrset_response_val.pstatefreq.fb_div =
                p_cli_request->pwrset_sub_command_args.pstatefreq.fb_div;
            p_cli_request->fetch_data.pwrset_response_val.pstatefreq.frac_div =
                p_cli_request->pwrset_sub_command_args.pstatefreq.frac_div;

            uint8_t cli_pstate = 0;
            uint32_t cli_freq_ctrl = 0;
            uint16_t cli_fb_div = 0;
            uint32_t cli_frac_div = 0;

            cli_pstate = p_cli_request->fetch_data.pwrset_response_val.pstatefreq.pstate;
            cli_freq_ctrl = p_cli_request->fetch_data.pwrset_response_val.pstatefreq.freq_ctrl;
            cli_fb_div = p_cli_request->fetch_data.pwrset_response_val.pstatefreq.fb_div;
            cli_frac_div = p_cli_request->fetch_data.pwrset_response_val.pstatefreq.frac_div;

            // Set the frequency for the pstate
            dvfs_pll_pstate_config_t pstate_config = {0};
            pstate_config.freq_ctrl_value = cli_freq_ctrl;
            pstate_config.dco_div_value = cli_fb_div;
            pstate_config.dco_frac_value = cli_frac_div;

            for (unsigned int core = 0; core < core_count; ++core)
            {
                const uintptr_t cluster_pex_base_addr =
                    (p_config->cluster_pex_base + (p_config->cluster_stride * core));
                const bool core_enabled = corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core);

                // Check if the core is valid and enabled
                if (corebits_is_bit_set(p_config->platform_cores_in_die, core) && core_enabled)
                {
                    dvfs_pll_set_pstate_config(cluster_pex_base_addr, cli_pstate, &pstate_config);

                    // below for debugging - or we may want to leave this in for testing.
                    dvfs_pll_pstate_config_t pstate_config_read = {0};
                    dvfs_pll_get_pstate_config(cluster_pex_base_addr, cli_pstate, &pstate_config_read);

                    printf("Read after write. Core = %d, pstate = %d, freq_ctrl = 0x%lx, fb_div = 0x%04x, "
                           "frac_div = 0x%lx\n",
                           core,
                           cli_pstate,
                           (unsigned long)pstate_config_read.freq_ctrl_value,
                           pstate_config_read.dco_div_value,
                           (unsigned long)pstate_config_read.dco_frac_value);
                    // end of code for debugging
                }
            }

            // // will complete request in callback
            DfwkAsyncRequestComplete(p_request);
            break;
        }

        default:
            DfwkAsyncRequestComplete(p_request);
            break;
        }
        break;
    }

    case CLI_COMMANDS_POWER_STATUS: {
        ppower_service_cli_request_t p_cli_request = (ppower_service_cli_request_t)p_request;
        p_cli_request->header = *p_request;

        p_cli_request->fetch_data.pwr_intparams.p_knobs = &p_runconfig->knobs;

        p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop_context = get_s_loop_context();
        p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop = get_s_ctrl_loop();

        p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_vr_telem_loop_context = get_s_vr_telem_loop_context();
        p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_telem_loop = get_s_telem_loop();

        p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_pvt_telem_loop_context = get_s_pvt_telem_loop_context();
        DfwkAsyncRequestComplete(p_request);
        break;
    }

    case CLI_COMMANDS_POWER_LOG: {
        DfwkAsyncRequestComplete(p_request);
        break;
    }

    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

void power_cli_requests_async_handler(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    // queue the request to the power loop to be done in idle
    power_loops_exec_in_idle(power_cli_requests_callback, p_request, p_context);
}
