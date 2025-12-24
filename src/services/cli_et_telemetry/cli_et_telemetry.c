//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_et_telemetry.c
 * CLI commands to test and debug event trace telemetry
 */

/*-------------------------------- Includes ---------------------------------*/
#include <FpFwCli.h>           // for FPFW_CLI_COMMAND, FpFwCliRegisterTable
#include <FpFwUtils.h>         // for FPFW_ARRAY_SIZE, FPFW_UNUSED
#include <bug_check.h>         // for BUG_CHECK
#include <cli_et_telemetry.h>  // for evt_telemetry_cli_init
#include <event_trace_relay.h> // for etr_service_get_context, etr_service_context_t
#include <inttypes.h>          // for PRIu64
#include <stdlib.h>            // for atoi
#include <string.h>            // for memset

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
static FPFW_CLI_STATUS evt_tel_cli_get_ddr_buff_info(int argc, const char** pp_argv);
static FPFW_CLI_STATUS evt_tel_cli_set_all_buf_freef(int argc, const char** pp_argv);
static FPFW_CLI_STATUS evt_tel_cli_get_evt_health(int argc, const char** pp_argv);

/*------------------- Declarations (Statics and globals) --------------------*/
static FPFW_CLI_COMMAND s_evt_tel_cli_cmd[] = {
    {NULL_LIST_ENTRY, "evt_tel", "get_buf_info", evt_tel_cli_get_ddr_buff_info, "Get DDR Buffer Info", "Usage: get_buf_info"},
    {NULL_LIST_ENTRY, "evt_tel", "set_buf_free", evt_tel_cli_set_all_buf_freef, "Free all DDR buffers", "Usage: set_buf_free"},
    {NULL_LIST_ENTRY, "evt_tel", "get_health", evt_tel_cli_get_evt_health, "Get Event Telemetry Health Stats", "Usage: get_health"},
};

static etr_service_context_t* p_etr_service_context = NULL;
/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

static FPFW_CLI_STATUS evt_tel_cli_get_ddr_buff_info(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    if (p_etr_service_context == NULL)
    {
        return CLI_ERROR;
    }

    unsigned int num_pending_asic_buffers = 0;
    unsigned int num_free_asic_buffers = 0;

    FpFwCliPrint("EVT Telemetry DDR Buffer Info:\n");

    for (uint16_t i = 0; i < ETR_DDR_BUFFERS_CAPACITY_MAX; i++)
    {
        char state = 'U';
        /* Print buffer information */
        switch (p_etr_service_context->ddr_buffers[i].state)
        {

        case ETR_DDR_BUFFER_STATE_FREE:
            num_free_asic_buffers++;
            state = 'F';
            break;
        case ETR_DDR_BUFFER_STATE_ACTIVE:
            num_pending_asic_buffers++;
            state = 'A';
            break;
        case ETR_DDR_BUFFER_STATE_PENDING:
            state = 'P';
            break;
        default:
            state = 'U';
            break;
        }
        FpFwCliPrint("%u(%c).T:%u, A:%p, S:%zu\n",
                     i,
                     state,
                     p_etr_service_context->ddr_buffers[i].type,
                     p_etr_service_context->ddr_buffers[i].payload_management.base_addr,
                     p_etr_service_context->ddr_buffers[i].payload_management.size_bytes);
    }

    FpFwCliPrint("Pending: %u\n", num_pending_asic_buffers);
    FpFwCliPrint("Free: %u\n", num_free_asic_buffers);

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS evt_tel_cli_set_all_buf_freef(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    if (p_etr_service_context == NULL)
    {
        return CLI_ERROR;
    }

    for (uint16_t i = 0; i < ETR_DDR_BUFFERS_CAPACITY_MAX; i++)
    {
        /* Don't touch the active buffer - can corrupt event trace */
        if (p_etr_service_context->ddr_buffers[i].state != ETR_DDR_BUFFER_STATE_ACTIVE)
        {
            p_etr_service_context->ddr_buffers[i].state = ETR_DDR_BUFFER_STATE_FREE;
        }
        else
        {
            FpFwCliPrint("Buffer %d is active and will be skipped\n", i);
        }
    }
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS evt_tel_cli_get_evt_health(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    if (p_etr_service_context == NULL)
    {
        return CLI_ERROR;
    }

    FpFwCliPrint("HSP Buffers Dropped: %" PRIu64 "\n", p_etr_service_context->health_stats.hsp_buffers_dropped);
    FpFwCliPrint("ASIC Buffers Re-used: %" PRIu64 "\n", p_etr_service_context->health_stats.asic_buffers_reused);
    FpFwCliPrint("Delayed Host Reads: %" PRIu64 "\n", p_etr_service_context->health_stats.delayed_host_reads);

    return CLI_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/

void evt_telemetry_cli_init(void)
{
    p_etr_service_context = get_etr_service_context();

    //! register the EVT Telemetry CLI commands
    FpFwCliRegisterTable(s_evt_tel_cli_cmd, FPFW_ARRAY_SIZE(s_evt_tel_cli_cmd));
}