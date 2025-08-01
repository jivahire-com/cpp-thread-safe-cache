//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_power_set.c
 * Source file with implementations of power set CLI commands.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <cli_power_common.h>
#include <cli_power_interface.h>
#include <cli_power_status.h>
#include <corebits.h>
#include <power_dfwk.h>
#include <power_runconfig.h>
#include <silibs_common.h>
#include <stdio.h>
#include <string.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-- Declarations (Statics and globals) --*/

// clang-format off
/* Structure storing the dictionary of sub-command string against its corresponding command id and callback function */
/* For now, since the set functions are only dummy calls, the completion callbacks are set to NULL. To be updated (ADO: 1887411)*/
const power_cli_sub_command_dictionary_element_t power_cli_set_sub_command_dictionary[] = {
    {"cap",            NULL, POWER_IF_CMD_SET_CAP               },
    {"desired",        NULL, POWER_IF_CMD_SET_DESIRED_PSTATE    },
    {"plimit",         NULL, POWER_IF_CMD_SET_PLIMIT            },
    {"loopdis",        NULL, POWER_IF_CMD_SET_LOOP_DISABLES     },
    {"minupdate",      NULL, POWER_IF_CMD_SET_MINUPDATE         },
    {"nominal",        NULL, POWER_IF_CMD_SET_NOMINAL           },
    {"racklimit",      NULL, POWER_IF_CMD_SET_RACK_LIMIT        },
    {"forced",         NULL, POWER_IF_CMD_SET_FORCED            },
    {"pstate_freq",    NULL, POWER_IF_CMD_SET_PSTATE_FREQ       },
    {"alarm",          NULL, POWER_IF_CMD_SET_ALARM_THRESHOLD   },
    {"force_pmin",     NULL, POWER_IF_CMD_SET_FORCE_PMIN        },
    {"soc_hot",        NULL, POWER_IF_CMD_SET_SOC_HOT           },
    {"mem_hot",        NULL, POWER_IF_CMD_SET_MEM_HOT           },
    {"therm_trip",     NULL, POWER_IF_CMD_SET_THERM_TRIP        },
    {"curr_throttle",  NULL, POWER_IF_CMD_SET_CURR_THROTTLE     },
};
//clang-format on

const uint32_t length_set_commands_dictionary =
    sizeof(power_cli_set_sub_command_dictionary) / sizeof(power_cli_sub_command_dictionary_element_t);

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/

power_if_cmd_t cli_power_set_get_cmd_id(char* sub_command)
{
    if (sub_command == NULL)
    {
        return POWER_IF_CMD_UNKNOWN;
    }

    /* Parse dictionary for subcommand and fetch corresponding power_ext_if_cmd_id */
    for (uint32_t index = 0; index < length_set_commands_dictionary; index++)
    {
        if (strcmp(sub_command, power_cli_set_sub_command_dictionary[index].sub_command) == 0)
        {
            return power_cli_set_sub_command_dictionary[index].power_ext_if_cmd_id;
        }
    }

    /* If sub-command is not found in the dictionary */
    return POWER_IF_CMD_UNKNOWN;
}

void cli_power_set_async_print(PDFWK_ASYNC_REQUEST_HEADER p_request, void* completion_context)
{

    FPFW_UNUSED(completion_context);

    ppower_service_cli_request_t p_cli_request = (ppower_service_cli_request_t)p_request;

    uint16_t value = 0;

    if (p_cli_request == NULL) {
        printf("Invalid request\n");
        return;
    }

    switch (p_cli_request->power_ext_if_cmd_id)
    {
        case POWER_IF_CMD_SET_CAP: {        
            if(p_cli_request->fetch_data.pwrset_response_val.pwr_icc_cap_result.result != MP_POWER_CAP_SUCCESS) {         

                   printf("Failed to set power cap: %d\n", 
                            p_cli_request->fetch_data.pwrset_response_val.pwr_icc_cap_result.result);
            }
            else {

                printf("  pwr_set cap: current - %dW,  previous %dW,  result - %d (%s)\n",
                p_cli_request->fetch_data.pwrset_response_val.pwr_icc_cap_result.current_cap, //current,
                p_cli_request->fetch_data.pwrset_response_val.pwr_icc_cap_result.previous_cap, //previous,
                p_cli_request->fetch_data.pwrset_response_val.pwr_icc_cap_result.result, //result,
                (p_cli_request->fetch_data.pwrset_response_val.pwr_icc_cap_result.result == MP_POWER_CAP_SUCCESS ? "success" : "fail"));

            }
            break;
        }

        case POWER_IF_CMD_SET_DESIRED_PSTATE: {
            printf("pwr set desired: core - %d (0x%08x) desired - 0x%x throttle_pri "
                    "- 0x%x\n",
                    p_cli_request->fetch_data.pwrset_response_val.desiredparams.core,
                    p_cli_request->fetch_data.pwrset_response_val.desiredparams.cluster_pex_base_addr,
                    p_cli_request->fetch_data.pwrset_response_val.desiredparams.state,
                    p_cli_request->fetch_data.pwrset_response_val.desiredparams.throttle);
             
            break;
        }

        case POWER_IF_CMD_SET_PLIMIT: {
            printf("\npwr set plimit: core - %d (0x%08x) plimit - 0x%x \n", 
                    p_cli_request->fetch_data.pwrset_response_val.plimitparams.core,
                    p_cli_request->fetch_data.pwrset_response_val.plimitparams.cluster_pex_base_addr,
                    p_cli_request->fetch_data.pwrset_response_val.plimitparams.state);

            break;
        }

        case POWER_IF_CMD_SET_LOOP_DISABLES: {
            value = p_cli_request->fetch_data.pwrset_response_val.loopdis_params.loopdis_bits;

            printf("\n loop disables: %x (%s%s%s%s)\n",
                   value,
                   ((value) ? "" : " "), // this line is here to make it possible to \b at the bottom
                   ((value & power_loops_disable_t_CTRL_LOOP) ? "ctrl " : ""),
                   ((value & power_loops_disable_t_VR_TELEM_LOOP) ? "vrtelem " : ""),
                   ((value & power_loops_disable_t_PVT_TELEM_LOOP) ? "pvttelem" : "\b"));

            break;
        }

        case POWER_IF_CMD_SET_MINUPDATE: {
            value = p_cli_request->fetch_data.pwrset_response_val.minupdate_val;

            if (value > PLIMIT_UPDATE_MAX)
            {
                printf("\n pwr set minupdate parameter out of range\n");
                return;
            }

            printf(" \nmin_plimit_update: %d", value);

            if (value == power_loops_minimum_plimit_t_NONE)
            {
                printf("\n NONE");
            }
            else
            {
                printf("\n%d", 1 << (value - 1));
            }    

            break;
        }

        case POWER_IF_CMD_SET_NOMINAL: {
            printf("  nominal pstate: P%d (previous P%d)\n", 
                p_cli_request->fetch_data.pwrset_response_val.nominalparams.current_val,
                p_cli_request->fetch_data.pwrset_response_val.nominalparams.previous_val);

            break;
        }

        case POWER_IF_CMD_SET_RACK_LIMIT: {
            printf("\n  rack_limit: %s \n", (p_cli_request->fetch_data.pwrset_response_val.racklimit) ? "asserted" : "not asserted");

            break;
        }

        case POWER_IF_CMD_SET_FORCED: {
            printf("\n  pwr set forced: pstate: %d, ldodacin - 0x%04x \n",
                    p_cli_request->fetch_data.pwrset_response_val.forcedparams.pstate,
                    p_cli_request->fetch_data.pwrset_response_val.forcedparams.ldodacin);
            break;
        }

        case POWER_IF_CMD_SET_PSTATE_FREQ: {
            printf("\n pwr set pstate table: pstate = %d, freq_ctrl = 0x%lx, fb_div = 0x%04x, frac_div = 0x%lx\n", 
                    p_cli_request->fetch_data.pwrset_response_val.pstatefreq.pstate,
                    (unsigned long)p_cli_request->fetch_data.pwrset_response_val.pstatefreq.freq_ctrl,
                    p_cli_request->fetch_data.pwrset_response_val.pstatefreq.fb_div,
                    (unsigned long)p_cli_request->fetch_data.pwrset_response_val.pstatefreq.frac_div);
            break;
        }
        case POWER_IF_CMD_SET_ALARM_THRESHOLD: {
            printf("pwr set alarm core[%d] Addr[0x%08x] Pex_grp[%d] alarm%c_cfg%d: alarm_thresh[%x] hist_thresh[%x] dual_die[%s]\n", 
                   p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.core,
                   p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.base_addr,
                   p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.pex_group,
                   p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.ab_selector,
                   p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.alarm_id,
                   p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.alarm_threshold,
                   p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.hist_threshold,
                   (p_cli_request->fetch_data.pwrset_response_val.alarm_cfg.dual_die ? "true" : "false"));
            break;
        }
        case POWER_IF_CMD_SET_FORCE_PMIN:
        case POWER_IF_CMD_SET_SOC_HOT:
        case POWER_IF_CMD_SET_MEM_HOT:
        case POWER_IF_CMD_SET_THERM_TRIP:
            break;
        case POWER_IF_CMD_SET_CURR_THROTTLE: {
            printf("pwr set curr_throttle: core - %d, threshold_1 - %d threshold_2 - %d threshold_3 - %d\n",
                   p_cli_request->fetch_data.pwrset_response_val.currthresh_params.core,
                   p_cli_request->fetch_data.pwrset_response_val.currthresh_params.curr_threshold_1,
                   p_cli_request->fetch_data.pwrset_response_val.currthresh_params.curr_threshold_2,
                   p_cli_request->fetch_data.pwrset_response_val.currthresh_params.curr_threshold_3);
            break;
        }
        default:
            break;
    }

    // Print completion message
    printf("pwr_cli_comp\n");
}

