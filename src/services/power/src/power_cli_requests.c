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
#include <dvfs_regs.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send, fpfw_icc_base_recv
#include <icc_platform_defines.h>
#include <inttypes.h> // for PRId32, PRIx32
#include <pex_regs.h>
#include <power_dfwk.h> // for (anonymous), ppower_service_cli_re...
#include <stdbool.h>    // for false
#include <stddef.h>

/*-- Symbolic Constant Macros (defines) --*/
#define CORE_CLUSTER_PEX_TOP_ALARM_CFG_OFFSET(alarm_num, ab_sel) \
    (0x280 + ((alarm_num) * 0x10) + ((ab_sel) == 'B' ? 0x4 : 0x0))
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void power_set_cap(uint16_t cap_value, ppower_service_cli_request_t p_request);

static void power_set_cap_callback(int result, uint16_t current, uint16_t previous);
static void power_set_minupdate(power_runconfig_t* p_runconfig, uint16_t minupdate_val);
static void power_cli_remote_die_send_complete_notify(void* context, fpfw_status_t status);
static fpfw_status_t power_cli_remote_die_send(power_runconfig_t* p_runconfig, ppower_service_cli_request_t p_cli_request);

/*-- Declarations (Statics and globals) --*/
static bool d2d_send_in_progress = false;
static remote_pwrcli_request_t d2d_cli_msg = {.icc_cmd_code = RMSS_D2D_MAILBOX_PWR_CLI_REQ};
static fpfw_icc_base_send_req_t d2d_send_params = {0};
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
    p_runconfig->knobs.minimum_plimit_updates = minupdate_val;
}

static void power_cli_remote_die_send_complete_notify(void* context, fpfw_status_t status)
{
    FPFW_RUNTIME_ASSERT(context != NULL);
    ppower_service_cli_request_t p_request = (ppower_service_cli_request_t)context;
    //! Check for errors
    if (status != DFWK_SUCCESS)
    {
        printf("[PWR CLI] D2D Send Failed: Status[0x%" PRIx32 "] Internal Status[0x%" PRIx32 "] Cmd[0x%x]\n",
               status,
               d2d_send_params.send_req.Output.Status,
               p_request->power_ext_if_cmd_id);
    }
    else
    {
        printf("[PWR CLI] D2D Send Completed Successfully for Cmd [0x%x]\n", p_request->power_ext_if_cmd_id);
        POWER_LOG_TRACE("[PWR CLI] Payload contents (hex): ");
        for (size_t i = 0; i < d2d_send_params.buffer_size / sizeof(uint32_t); ++i)
        {
            POWER_LOG_TRACE("Word %zu: 0x%lx\n", i, ((uint32_t*)d2d_send_params.payload_buffer)[i]);
        }

        switch (p_request->power_ext_if_cmd_id)
        {
        case POWER_IF_CMD_SET_LOOP_DISABLES:
            printf("[PWR CLI] Remote die loop disables Cmd Sent successfully.\n");
            //! complete the request
            DfwkAsyncRequestComplete(&p_request->header);
            break;
        // Add other cases as needed for different command IDs
        default:
            printf("[PWR CLI] Remote die command [0x%x] not supported!\n", p_request->power_ext_if_cmd_id);
            break;
        }
    }
    d2d_send_in_progress = false;
}

static fpfw_status_t power_cli_remote_die_send(power_runconfig_t* p_runconfig, ppower_service_cli_request_t p_cli_request)
{
    FPFW_RUNTIME_ASSERT(p_runconfig != NULL);
    FPFW_RUNTIME_ASSERT(p_cli_request != NULL);

    if (d2d_send_in_progress)
    {
        printf("[PWR CLI] D2D send already in progress, cannot send another request.");
        return FPFW_STATUS_BUSY;
    }
    d2d_send_in_progress = true;

    //! clear previous send message
    memset(&d2d_cli_msg, 0, sizeof(d2d_cli_msg));
    memset(&d2d_send_params, 0, sizeof(d2d_send_params));

    // Initialize the remote request structure
    d2d_cli_msg.icc_cmd_code = RMSS_D2D_MAILBOX_PWR_CLI_REQ;
    d2d_cli_msg.power_ext_if_cmd_id = p_cli_request->power_ext_if_cmd_id;
    memcpy(&d2d_cli_msg.pwrset_sub_command_args, &p_cli_request->pwrset_sub_command_args, sizeof(d2d_cli_msg.pwrset_sub_command_args));

    //! Prepare send request
    d2d_send_params.payload_buffer = &d2d_cli_msg;
    d2d_send_params.buffer_size = sizeof(d2d_cli_msg);
    d2d_send_params.cb = power_cli_remote_die_send_complete_notify;
    d2d_send_params.cb_ctx = p_cli_request; // pass the request context for completion callback

    size_t num_words = sizeof(d2d_cli_msg) / sizeof(uint32_t);
    POWER_LOG_TRACE("[PWR CLI] Payload contents (hex): ");
    for (size_t i = 0; i < num_words; ++i)
    {
        POWER_LOG_TRACE("Word %zu: 0x%lx\n", i, d2d_cli_msg.as_uint32[i]);
    }
    //! Send the payload
    return fpfw_icc_base_send((fpfw_icc_base_ctx_t*)(p_runconfig->p_sconfig->icc_d2d_cli_ctx), &d2d_send_params);
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
                vptr_dvfs_nonsecure_cppc_desired_perf cppc_desired_perf =
                    (vptr_dvfs_nonsecure_cppc_desired_perf)(cluster_pex_base_addr + PEX_DVFS_NON_ADDRESS +
                                                            DVFS_NONSECURE_CPPC_DESIRED_PERF_ADDRESS);
                vptr_dvfs_dvfs_fsm_sr fsm_sr =
                    (vptr_dvfs_dvfs_fsm_sr)(cluster_pex_base_addr + PEX_DVFS_ROOT_ADDRESS + DVFS_DVFS_FSM_SR_ADDRESS);
                if (core < core_count) // NUM_AP_CORES_PER_DIE) {
                {
                    printf(
                        "\nCore[%d]: CPPC_DESIRED_PERF addr = 0x%x\nBefore: cppc_value = 0x%x, base_perf = "
                        "0x%x, boost_pri = 0x%x, mpam_h = 0x%x, throttle_pri = 0x%x",
                        core,
                        (uintptr_t)cppc_desired_perf,
                        cppc_desired_perf->cppc_value,
                        cppc_desired_perf->base_perf,
                        cppc_desired_perf->boost_pri,
                        cppc_desired_perf->mpam_h,
                        cppc_desired_perf->throttle_pri);

                    // Set the CPPC desired performance for the core, should effect the DVFS FSM SR
                    dvfs_ns_set_cppc_desired(cluster_pex_base_addr, desired, throttle);
                    printf("After: cppc_value = 0x%x, base_perf = 0x%x, boost_pri = 0x%x, mpam_h = 0x%x, "
                           "throttle_pri = 0x%x\n",
                           cppc_desired_perf->cppc_value,
                           cppc_desired_perf->base_perf,
                           cppc_desired_perf->boost_pri,
                           cppc_desired_perf->mpam_h,
                           cppc_desired_perf->throttle_pri);

                    // Check the DVFS FSM SR to see if the cppc_desired_pstate is set correctly
                    printf("DVFS_FSM_SR addr = 0x%x, cppc_desired_pstate = 0x%x, vfsm_pstate = 0x%x\n",
                           (uintptr_t)fsm_sr,
                           fsm_sr->cppc_desired_pstate,
                           fsm_sr->vfsm_pstate);

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
                vptr_dvfs_plimit plimit_reg =
                    (vptr_dvfs_plimit)(cluster_pex_base_addr + PEX_DVFS_ROOT_ADDRESS + DVFS_PLIMIT_ADDRESS);
                vptr_dvfs_dvfs_fsm_sr fsm_sr =
                    (vptr_dvfs_dvfs_fsm_sr)(cluster_pex_base_addr + PEX_DVFS_ROOT_ADDRESS + DVFS_DVFS_FSM_SR_ADDRESS);
                if ((core < core_count) && corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core))
                {
                    dvfs_plimit plimit_as_uint32 = {
                        .vf_index = desired,
                        .power_cap = false,
                        .currthresh_1 = plimit_reg->currthresh_1,
                        .currthresh_2 = plimit_reg->currthresh_2,
                        .currthresh_3 = plimit_reg->currthresh_3,
                    };
                    printf("\nCore[%d]: PLIMIT addr = 0x%x\nBefore: vf_index = 0x%x power_cap = 0x%x\n",
                           core,
                           (uintptr_t)plimit_reg,
                           plimit_reg->vf_index,
                           plimit_reg->power_cap);

                    // Set the PLIMIT for the core
                    power_set_plimit(p_runconfig, core, plimit_as_uint32);
                    printf("After: vf_index = 0x%x power_cap = 0x%x\n", plimit_reg->vf_index, plimit_reg->power_cap);

                    // Check the DVFS FSM SR to see if the vfsm pstate is set correctly
                    printf("DVFS_FSM_SR addr = 0x%x, cppc_desired_pstate = 0x%x, vfsm_pstate = 0x%x\n",
                           (uintptr_t)&fsm_sr,
                           fsm_sr->cppc_desired_pstate,
                           fsm_sr->vfsm_pstate);

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

            DfwkAsyncRequestComplete(p_request);
            break;
        }

        case POWER_IF_CMD_SET_LOOP_DISABLES: {
            value = p_runconfig->knobs.loops_disable = p_cli_request->pwrset_sub_command_args.loopdis_params.loopdis_bits;
            p_cli_request->fetch_data.pwrset_response_val.loopdis_params.loopdis_bits = value;
            p_cli_request->fetch_data.pwrset_response_val.loopdis_params.mode =
                p_cli_request->pwrset_sub_command_args.loopdis_params.mode;

            if (p_cli_request->pwrset_sub_command_args.loopdis_params.mode == LOOP_DISABLE_MODE_DUAL_DIE)
            {
                printf("[PWR CLI] [POWER_IF_CMD_SET_LOOP_DISABLES] Dual Die\n");
                fpfw_status_t status = power_cli_remote_die_send(p_runconfig, p_cli_request);
                if (status != FPFW_STATUS_SUCCESS)
                {
                    printf("[PWR CLI] POWER_IF_CMD_SET_LOOP_DISABLES power_cli_remote_die_send failed: "
                           "0x%" PRIx32 "\n",
                           status);
                }
            }
            else
            {
                printf("[PWR CLI] [POWER_IF_CMD_SET_LOOP_DISABLES] Single Die\n");
                //! complete the request
                DfwkAsyncRequestComplete(p_request);
            }
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
            DfwkAsyncRequestComplete(p_request);
            break;
        }

        case POWER_IF_CMD_SET_ALARM_THRESHOLD: {
            all = p_cli_request->pwrset_sub_command_args.alarm_cfg.all;
            core = p_cli_request->pwrset_sub_command_args.alarm_cfg.core;
            uint16_t alarm_thresh = p_cli_request->pwrset_sub_command_args.alarm_cfg.alarm_threshold;
            uint16_t hist_thresh = p_cli_request->pwrset_sub_command_args.alarm_cfg.hist_threshold;
            char alarm_type = p_cli_request->pwrset_sub_command_args.alarm_cfg.ab_selector;
            bool dual_die = p_cli_request->pwrset_sub_command_args.alarm_cfg.dual_die;
            uint8_t alarm_id = p_cli_request->pwrset_sub_command_args.alarm_cfg.alarm_id;
            uint8_t pex_group = p_cli_request->pwrset_sub_command_args.alarm_cfg.pex_group;

            do
            {
                //! figure out the base address for the alarm chosen
                const uintptr_t cluster_pex_base_addr =
                    (p_config->cluster_pex_base + (p_config->cluster_stride * core));
                uintptr_t dvfs_throt_sr = cluster_pex_base_addr + PEX_DVFS_ROOT_ADDRESS + DVFS_DVFS_THROT_SR_ADDRESS;
                uintptr_t alarm_base_addr =
                    cluster_pex_base_addr + PEX_PVT_ADDRESS +
                    (pex_group == 0 ? PEX_PVTC_RDL_REGMAP_PEX_G0_ADDRESS : PEX_PVTC_RDL_REGMAP_PEX_G1_ADDRESS) +
                    CORE_CLUSTER_PEX_TOP_ALARM_CFG_OFFSET(alarm_id, alarm_type);
                vptr_pex_g0_alarma_cfg_0 pex_alarm_cfg = (vptr_pex_g0_alarma_cfg_0)alarm_base_addr;

                if (core < core_count) // NUM_AP_CORES_PER_DIE) {
                {
                    printf(
                        "Alarm Config: core=%u, pex_group=%u, alarm_id=%u, type=%c, alarm_cfg_addr=0x%lx\n",
                        (unsigned int)core,
                        (unsigned int)pex_group,
                        (unsigned int)alarm_id,
                        alarm_type,
                        (unsigned long)alarm_base_addr);

                    printf("Current alarm_thresh=0x%04x, hist_thresh=0x%04x\n",
                           (unsigned int)pex_alarm_cfg->alarm_thresh,
                           (unsigned int)pex_alarm_cfg->hist_thresh);

                    //! Print the current status of dvfs throt sr
                    parse_dvfs_throttle_status(dvfs_throt_sr);

                    //! Set the thresholds
                    pex_alarm_cfg->hist_thresh = hist_thresh;
                    pex_alarm_cfg->alarm_thresh = alarm_thresh;

                    //! fill in the response params
                    p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.core = core;
                    p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.alarm_threshold = alarm_thresh;
                    p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.hist_threshold = hist_thresh;
                    p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.ab_selector = alarm_type;
                    p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.pex_group = pex_group;
                    p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.alarm_id = alarm_id;
                    p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.dual_die = dual_die;
                    p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.base_addr = alarm_base_addr;
                    printf("Set alarm_thresh=0x%04x, hist_thresh=0x%04x\n",
                           (unsigned int)pex_alarm_cfg->alarm_thresh,
                           (unsigned int)pex_alarm_cfg->hist_thresh);

                    // Insert a small delay to ensure hardware registers settle after setting thresholds
                    for (volatile int delay = 0; delay < 10000; ++delay)
                    {
                        // simple busy-wait loop
                    }
                    //! Print the current status of dvfs throt sr post setting the alarm
                    parse_dvfs_throttle_status(dvfs_throt_sr);
                }
                else
                {
                    p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.core = core;
                }
                ++core;

            } while (all && core < core_count); // power_context.core_count);
            DfwkAsyncRequestComplete(p_request);
            break;
        }
        case POWER_IF_CMD_SET_FORCE_PMIN: {
            power_pmin_type_t pmin_type = (power_pmin_type_t)p_cli_request->pwrset_sub_command_args.pmin_type;
            if ((pmin_type > PM_PMIN_ALL) && (pmin_type <= PM_FW_PMIN_CONTROL))
            {
                printf("[PWR CLI] Forcing PMIN type: %d\n", pmin_type);
                power_hw_force_pmin(pmin_type);
            }
            if (pmin_type == PM_PMIN_ALL)
            {
                printf("[PWR CLI] Clearing Force Pmin\n");
                power_hw_clear_force_pmin(PM_PMIN_ALL);
            }
            DfwkAsyncRequestComplete(p_request);
            break;
        }
        case POWER_IF_CMD_SET_SOC_HOT: {
            bool state = p_cli_request->pwrset_sub_command_args.io_thermal;
            if (state)
            {
                printf("[PWR CLI] Setting SOC_HOT state to ON\n");
            }
            else
            {
                printf("[PWR CLI] Setting SOC_HOT state to OFF\n");
            }
            power_hw_set_thermal_io_state(SOC_HOT, state);
            DfwkAsyncRequestComplete(p_request);
            break;
        }
        case POWER_IF_CMD_SET_MEM_HOT: {
            bool state = p_cli_request->pwrset_sub_command_args.io_thermal;
            if (state)
            {
                printf("[PWR CLI] Setting MEM_HOT state to ON\n");
            }
            else
            {
                printf("[PWR CLI] Setting MEM_HOT state to OFF\n");
            }
            power_hw_set_thermal_io_state(MEM_HOT, state);
            DfwkAsyncRequestComplete(p_request);
            break;
        }
        case POWER_IF_CMD_SET_THERM_TRIP: {
            bool state = p_cli_request->pwrset_sub_command_args.io_thermal;
            if (state)
            {
                printf("[PWR CLI] Setting THERM_TRIP state to ON\n");
            }
            else
            {
                printf("[PWR CLI] Setting THERM_TRIP state to OFF\n");
            }
            power_hw_set_thermal_io_state(THERM_TRIP, state);
            DfwkAsyncRequestComplete(p_request);
            break;
        }
        case POWER_IF_CMD_SET_CURR_THROTTLE: {
            p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop = get_s_ctrl_loop();
            core = p_cli_request->pwrset_sub_command_args.currthresh_params.core;
            all = p_cli_request->pwrset_sub_command_args.currthresh_params.all;

            do
            {
                const uintptr_t cluster_pex_base_addr =
                    (p_config->cluster_pex_base + (p_config->cluster_stride * core));
                vptr_dvfs_plimit plimit_reg =
                    (vptr_dvfs_plimit)(cluster_pex_base_addr + PEX_DVFS_ROOT_ADDRESS + DVFS_PLIMIT_ADDRESS);

                printf("Current PLIMIT register values for core %d:\n", core);
                printf("  vf_index = 0x%x\n", plimit_reg->vf_index);
                printf("  power_cap = 0x%x\n", plimit_reg->power_cap);
                printf("  currthresh_1 = 0x%x\n", plimit_reg->currthresh_1);
                printf("  currthresh_2 = 0x%x\n", plimit_reg->currthresh_2);
                printf("  currthresh_3 = 0x%x\n", plimit_reg->currthresh_3);

                if ((core < core_count) && corebits_is_bit_set(&p_runconfig->fuses.valid_cores, core))
                {
                    p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop->cores.core[core].plimit_t1 =
                        p_cli_request->pwrset_sub_command_args.currthresh_params.curr_threshold_1;
                    p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop->cores.core[core].plimit_t2 =
                        p_cli_request->pwrset_sub_command_args.currthresh_params.curr_threshold_2;
                    p_cli_request->fetch_data.pwr_intparams.p_pwrstatus_s_ctrl_loop->cores.core[core].plimit_t3 =
                        p_cli_request->pwrset_sub_command_args.currthresh_params.curr_threshold_3;

                    p_cli_request->fetch_data.pwrset_response_val.currthresh_params.core = core;
                    p_cli_request->fetch_data.pwrset_response_val.currthresh_params.curr_threshold_1 =
                        p_cli_request->pwrset_sub_command_args.currthresh_params.curr_threshold_1;
                    p_cli_request->fetch_data.pwrset_response_val.currthresh_params.curr_threshold_2 =
                        p_cli_request->pwrset_sub_command_args.currthresh_params.curr_threshold_2;
                    p_cli_request->fetch_data.pwrset_response_val.currthresh_params.curr_threshold_3 =
                        p_cli_request->pwrset_sub_command_args.currthresh_params.curr_threshold_3;
                }
                else
                {
                    p_cli_request->fetch_data.pwrset_response_val.currthresh_params.core = core;
                }
            } while (all && ++core < core_count);

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
