//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scp_avs_cli.c
 * This file contains the implementation of the AVS CLI (client) module
 * and related functionality.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h> // for DfwkAsyncRequestInititalize, PDFWK_INTER...
#include <FpFwCli.h>
#include <FpFwLinkedList.h>
#include <FpFwUtils.h>
#include <idsw_kng.h>
#include <scp_avs_cli.h>    // for avs_client_init_completion_routine, pavs...
#include <scp_avs_driver.h> // for scp_avs_request_t

/*-- Symbolic Constant Macros (defines) --*/
#define MAX_CLI_VOLTAGE         1000 // mV
#define MAX_CLI_MULTI_READ_CMDS 6

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS scp_avs_read_data_cli(int argc, const char** argv);
static FPFW_CLI_STATUS scp_avs_write_data_cli(int argc, const char** argv);
static FPFW_CLI_STATUS scp_avs_read_vct_cli(int argc, const char** argv);
static FPFW_CLI_STATUS scp_avs_read_multi_cli(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/
static uint8_t max_num_avs_bus = 0;
typedef struct _avs_cli_request_context_t
{
    scp_avs_request_t request;
    bool in_use;
} avs_cli_request_context_t;
avs_cli_request_context_t cli_avs_request = {0};

pscp_avs_interface_t cli_avs_interfaces[MAX_AVS_INST] = {0};

static FPFW_CLI_COMMAND scp_avs_cli_list[] = {
    {NULL_LIST_ENTRY, "avs", "avs_read", scp_avs_read_data_cli, "AVS read data", "Usage: avs_read <avs bus number> <rail number> <read command type>"},
    {NULL_LIST_ENTRY, "avs", "avs_write", scp_avs_write_data_cli, "AVS write data", "Usage: avs_write <avs bus number> <rail number> <write command type> <data>"},
    {NULL_LIST_ENTRY, "avs", "avs_read_all", scp_avs_read_vct_cli, "AVS read VCT", "Usage: avs_read_all <avs bus number> <rail number>"},
    {NULL_LIST_ENTRY, "avs", "avs_read_m", scp_avs_read_multi_cli, "AVS read multi", "Usage: avs_read_multi <avs bus number> <command count> <rail> <cmd> <rail> <cmd>... Max CLI cmds accepted = 6"},
};

/*------------- Functions ----------------*/
bool check_not_in_use(void)
{
    if (cli_avs_request.in_use)
    {
        // TODO: Add Event Trace for AVS. https://azurecsi.visualstudio.com/Dev/_workitems/edit/1910297
        FpFwCliPrint("\n AVSBus CLI not complete, rejecting new CLI command\n");
        return false;
    }
    return true;
}

void AVSCLIRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(Request);

    int cli_avs_bus = (int)CompletionContext;
    cli_avs_request.in_use = false;
    uint8_t scp_cmd_count = 0;

    if (cli_avs_request.request.avs_response_status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("\n AVS CLI status error = %x\n", cli_avs_request.request.avs_response_status);
        return;
    }

    switch (cli_avs_request.request.Header.RequestType)
    {
    case AVS_REQUEST_WRITE_DATA:
        FpFwCliPrint("\n AVS CLI Write complete\n");
        FpFwCliPrint("\n AVS voltage read.\n AVSBus = %d\n rail = %d\n AVS volt. = %d.%03d\n",
                     cli_avs_bus,
                     cli_avs_request.request.avs_params.avs_cmd_info.rail_id,
                     ((cli_avs_request.request.avs_response_single_resp) / 1000),
                     ((cli_avs_request.request.avs_response_single_resp) % 1000));
        break;

    case AVS_REQUEST_READ_DATA:
        switch (cli_avs_request.request.avs_params.avs_cmd_info.cmd_type)
        {
        case AVS_VOLTAGE_RW:
            FpFwCliPrint("\n AVS voltage read.\n AVSBus = %d\n rail = %d\n AVS volt. = %d.%03d\n",
                         cli_avs_bus,
                         cli_avs_request.request.avs_params.avs_cmd_info.rail_id,
                         ((cli_avs_request.request.avs_response_single_resp) / 1000),
                         ((cli_avs_request.request.avs_response_single_resp) % 1000));
            break;

        case AVS_CURRENT_READ:
            FpFwCliPrint("\n AVS current read.\n AVSBus = %d\n rail = %d\n AVS current = %d.%02d\n",
                         cli_avs_bus,
                         cli_avs_request.request.avs_params.avs_cmd_info.rail_id,
                         ((cli_avs_request.request.avs_response_single_resp) / 100),
                         ((cli_avs_request.request.avs_response_single_resp) % 100));
            break;

        case AVS_TEMPERATURE_READ:
            FpFwCliPrint(
                "\n AVS temperature read.\n AVSBus = %d\n rail = %d\n AVS temperature_dC = %d.%01d\n",
                cli_avs_bus,
                cli_avs_request.request.avs_params.avs_cmd_info.rail_id,
                ((cli_avs_request.request.avs_response_single_resp) / 10),
                ((cli_avs_request.request.avs_response_single_resp) % 10));
            break;

        default:
            FpFwCliPrint(" AVS CLI default\n");
            break;
        }
        break;

    case AVS_REQUEST_READ_ALL_VCT:
        FpFwCliPrint("\n AVS read VCT.\n  AVSBus = %d\n  rail = %d\n  AVS volt. = %d.%03d\n  AVS current = "
                     "%d.%02d\n  AVS temperature_dC = %d.%01d\n\n",
                     cli_avs_bus,
                     cli_avs_request.request.avs_params.avs_cmd_info.rail_id,
                     ((cli_avs_request.request.avs_response_vct.voltage_mV) / 1000),
                     ((cli_avs_request.request.avs_response_vct.voltage_mV) % 1000),
                     ((cli_avs_request.request.avs_response_vct.current_cA) / 100),
                     ((cli_avs_request.request.avs_response_vct.current_cA) % 100),
                     ((cli_avs_request.request.avs_response_vct.temperature_dC) / 10),
                     ((cli_avs_request.request.avs_response_vct.temperature_dC) % 10));
        break;

    case AVS_REQUEST_READ_MULTI:
        scp_cmd_count = cli_avs_request.request.avs_params.cmd_count;
        FpFwCliPrint("\n AVS read multi, AVSBus = %d, cmd_count = %d\n", cli_avs_bus, scp_cmd_count);
        for (uint8_t scp_avs_resp_idx = 0; scp_avs_resp_idx < scp_cmd_count; scp_avs_resp_idx++)
        {
            FpFwCliPrint(" data[%d]: 0x%0x\n",
                         (int16_t)scp_avs_resp_idx,
                         cli_avs_request.request.avs_resp_multi.avs_response_multi[scp_avs_resp_idx].data);
        }
        break;

    default:
        FpFwCliPrint(" AVSCLIRequestCompletion default\n");
        break;
    }
}

static FPFW_CLI_STATUS scp_avs_read_data_cli(int argc, const char** argv)
{
    FpFwCliPrint("\nscp_avs_read_data_cli func. call\n\n");

    if (check_not_in_use() && (argc == 4))
    {
        int avs_bus = atoi(argv[1]);
        if (avs_bus < 0 || avs_bus >= max_num_avs_bus)
        {
            FpFwCliPrint("ERROR! Invalid Arg (avs_bus) \n");
            return CLI_ERROR;
        }

        int rail_sel = atoi(argv[2]);
        if (rail_sel < 0 || rail_sel >= MAX_AVS_RAILS)
        {
            FpFwCliPrint("ERROR! Invalid Arg (rail_sel) \n");
            return CLI_ERROR;
        }
        AVS_CMD_DATA_TYPE cmd_type = atoi(argv[3]);

        cli_avs_request.request.avs_params.avs_cmd_info.rail_id = rail_sel;
        cli_avs_request.request.avs_params.avs_cmd_info.cmd_type = cmd_type;
        cli_avs_request.in_use = true;
        scp_avs_client_read(&cli_avs_interfaces[avs_bus]->Header,
                            &cli_avs_request.request.Header,
                            AVSCLIRequestCompletion,
                            (void*)avs_bus);
    }
    else
    {
        FpFwCliPrint(" AVS read CLI Help\n");
        FpFwCliPrint("Cmds: 3, <avs_bus> <rail_sel> <cmd_type>\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS scp_avs_write_data_cli(int argc, const char** argv)
{
    FpFwCliPrint("\nscp_avs_write_data_cli func. call\n\n");

    if (check_not_in_use() && (argc == 5))
    {
        int avs_bus = atoi(argv[1]);
        if (avs_bus < 0 || avs_bus >= max_num_avs_bus)
        {
            FpFwCliPrint("ERROR! Invalid Arg (avs_bus) \n");
            return CLI_ERROR;
        }

        int rail_sel = atoi(argv[2]);
        if (rail_sel < 0 || rail_sel >= MAX_AVS_RAILS)
        {
            FpFwCliPrint("ERROR! Invalid Arg (rail_sel) \n");
            return CLI_ERROR;
        }
        AVS_CMD_DATA_TYPE cmd_type = atoi(argv[3]);

        uint32_t vr_data = (uint32_t)atoi(argv[4]);
        FpFwCliPrint("\nAVS write data = %u \n", vr_data);
        if (vr_data > MAX_CLI_VOLTAGE)
        {
            FpFwCliPrint("ERROR! Invalid Arg (vr_data > MAX) \n");
            return CLI_ERROR;
        }
        cli_avs_request.request.avs_params.avs_cmd_info.rail_id = rail_sel;
        cli_avs_request.request.avs_params.avs_cmd_info.cmd_type = cmd_type;
        cli_avs_request.request.avs_params.avs_data = vr_data;
        cli_avs_request.in_use = true;

        scp_avs_client_write(&cli_avs_interfaces[avs_bus]->Header,
                             &cli_avs_request.request.Header,
                             AVSCLIRequestCompletion,
                             (void*)avs_bus);
    }
    else
    {
        FpFwCliPrint(" AVS write CLI Help\n");
        FpFwCliPrint("Cmds: 4, <avs_bus> <rail_sel> <cmd_type> <vr_data>\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS scp_avs_read_vct_cli(int argc, const char** argv)
{
    FpFwCliPrint("\nscp_avs_read_vct_cli func. call\n\n");

    if (check_not_in_use() && (argc == 3))
    {
        int avs_bus = atoi(argv[1]);
        if (avs_bus < 0 || avs_bus >= max_num_avs_bus)
        {
            FpFwCliPrint("ERROR! Invalid Arg (avs_bus) \n");
            return CLI_ERROR;
        }

        int rail_sel = atoi(argv[2]);
        if (rail_sel < 0 || rail_sel >= MAX_AVS_RAILS)
        {
            FpFwCliPrint("ERROR! Invalid Arg (rail_sel) \n");
            return CLI_ERROR;
        }
        AVS_CMD_DATA_TYPE cmd_type = AVS_VOLTAGE_RW; // setting to the voltage read as the default for the first read.

        cli_avs_request.request.avs_params.avs_cmd_info.rail_id = rail_sel;
        cli_avs_request.request.avs_params.avs_cmd_info.cmd_type = cmd_type;
        cli_avs_request.in_use = true;

        scp_avs_client_read_all(&cli_avs_interfaces[avs_bus]->Header,
                                &cli_avs_request.request.Header,
                                AVSCLIRequestCompletion,
                                (void*)avs_bus);
    }
    else
    {
        FpFwCliPrint(" AVS read CLI Help\n");
        FpFwCliPrint("Cmds: 2, <avs_bus> <rail_sel>\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS scp_avs_read_multi_cli(int argc, const char** argv)
{
    FpFwCliPrint("\nscp_avs_read_multi_cli func. call\n\n");

    if (check_not_in_use() && (argc > 4))
    {
        int avs_bus = atoi(argv[1]);
        if (avs_bus < 0 || avs_bus >= max_num_avs_bus)
        {
            FpFwCliPrint("ERROR! Invalid Arg (avs_bus) \n");
            return CLI_ERROR;
        }

        int command_count = atoi(argv[2]);
        if (command_count > MAX_CLI_MULTI_READ_CMDS || command_count < 1)
        {
            FpFwCliPrint("ERROR! Invalid Arg (command_count range is 1 - 6) \n");
            return CLI_ERROR;
        }

        int rail_sel;
        AVS_CMD_DATA_TYPE cmd_type;

        uint8_t i;
        uint8_t atoi_index_offset;
        FpFwCliPrint("command_count = %0x\n", command_count);
        for (i = 0, atoi_index_offset = 0; i < command_count; i++, atoi_index_offset += 2)
        {
            rail_sel = atoi(argv[(3 + atoi_index_offset)]);
            if (rail_sel < 0 || rail_sel >= MAX_AVS_RAILS)
            {
                FpFwCliPrint("ERROR! Invalid Arg (rail_sel) \n");
                return CLI_ERROR;
            }
            cmd_type = atoi(argv[(4 + atoi_index_offset)]);
            cli_avs_request.request.avs_params.avs_cmd_array[i].rail_id = rail_sel;
            cli_avs_request.request.avs_params.avs_cmd_array[i].cmd_type = cmd_type;
            FpFwCliPrint("rail = %0x, cmd = %0x\n", rail_sel, cmd_type);
        }

        cli_avs_request.in_use = true;
        scp_avs_client_read_multi(&cli_avs_interfaces[avs_bus]->Header,
                                  &cli_avs_request.request.Header,
                                  AVSCLIRequestCompletion,
                                  (void*)avs_bus,
                                  (uint8_t)command_count);
    }
    else
    {
        FpFwCliPrint(" AVS read multi CLI Help\n");
        FpFwCliPrint("Cmds: 4+, <avs_bus> <cmd_count> <rail_sel> <cmd_type>...\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

void scp_avs_cli_initialize(pscp_avs_interface_t avs_array[])
{
    FpFwCliRegisterTable(scp_avs_cli_list, FPFW_ARRAY_SIZE(scp_avs_cli_list));

    KNG_DIE_ID current_die = idsw_get_die_id();
    max_num_avs_bus = ((current_die == DIE_0) ? 4 : 1); // DIE_1 only has one AVSBus.

    for (uint8_t i = 0; i < max_num_avs_bus; i++)
    {
        cli_avs_interfaces[i] = avs_array[i];
    }

    //
    // Only need to initialize the async request once here.
    //
    DfwkAsyncRequestInititalize((PDFWK_ASYNC_REQUEST_HEADER)&cli_avs_request.request.Header,
                                sizeof(cli_avs_request.request));
    FpFwCliPrint(" AVS CLI init complete\n");
}
