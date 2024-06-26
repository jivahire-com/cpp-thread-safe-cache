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
#include <scp_avs_cli.h>    // for avs_client_init_completion_routine, pavs...
#include <scp_avs_driver.h> // for scp_avs_request_t

/*-- Symbolic Constant Macros (defines) --*/

// TODO: Initialize the CLI for the remaining AVS buses.  AVS_BUS1 - AVS_BUS3 for Die0, and AVS_BUS4 for Die1.
// https://azurecsi.visualstudio.com/Dev/_workitems/edit/1484956
// This represents the AVS CLI buses initialized. This first PR only has AVS0 initialized.
#define MAX_AVS_INST    1
#define MAX_AVS_RAILS   2
#define MAX_CLI_VOLTAGE 1000 // mV

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS scp_avs_read_data_cli(int argc, const char** argv);
static FPFW_CLI_STATUS scp_avs_write_data_cli(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/
scp_avs_request_t cli_avs_request;
pscp_avs_interface_t cli_avs_interface;

static FPFW_CLI_COMMAND scp_avs_cli_list[] = {
    {NULL_LIST_ENTRY, "avs", "avs_read", scp_avs_read_data_cli, "AVS read data", "Usage: avs_read <avs bus number> <rail number> <read command type>"},
    {NULL_LIST_ENTRY, "avs", "avs_write", scp_avs_write_data_cli, "AVS write data", "Usage: avs_write <avs bus number> <rail number> <write command type> <data>"},
};

/*------------- Functions ----------------*/
void AVSCLIRequestCompletion(PDFWK_ASYNC_REQUEST_HEADER Request, void* CompletionContext)
{
    FPFW_UNUSED(CompletionContext);
    FPFW_UNUSED(Request);

    if (cli_avs_request.avs_response_status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("\n AVS CLI status error = %x\n", cli_avs_request.avs_response_status);
        return;
    }

    switch (cli_avs_request.Header.RequestType)
    {
    case AVS_REQUEST_WRITE_DATA:
        FpFwCliPrint("\n AVS CLI Write complete\n");
        FpFwCliPrint("\n AVS voltage read.\n AVSBus = %d\n rail = %d\n AVS volt. = %d.%03d\n",
                     cli_avs_interface->Device->avs_bus_num,
                     cli_avs_request.avs_params.rail_id,
                     ((cli_avs_request.avs_response_single_resp) / 1000),
                     ((cli_avs_request.avs_response_single_resp) % 1000));
        break;

    case AVS_REQUEST_READ_DATA:
        switch (cli_avs_request.avs_params.cmd_type)
        {
        case AVS_VOLTAGE_RW:
            FpFwCliPrint("\n AVS voltage read.\n AVSBus = %d\n rail = %d\n AVS volt. = %d.%03d\n",
                         cli_avs_interface->Device->avs_bus_num,
                         cli_avs_request.avs_params.rail_id,
                         ((cli_avs_request.avs_response_single_resp) / 1000),
                         ((cli_avs_request.avs_response_single_resp) % 1000));
            break;

        case AVS_CURRENT_READ:
            FpFwCliPrint("\n AVS current read.\n AVSBus = %d\n rail = %d\n AVS current = %d.%02d\n",
                         cli_avs_interface->Device->avs_bus_num,
                         cli_avs_request.avs_params.rail_id,
                         ((cli_avs_request.avs_response_single_resp) / 100),
                         ((cli_avs_request.avs_response_single_resp) % 100));
            break;

        case AVS_TEMPERATURE_READ:
            FpFwCliPrint(
                "\n AVS temperature read.\n AVSBus = %d\n rail = %d\n AVS temperature_dC = %d.%01d\n",
                cli_avs_interface->Device->avs_bus_num,
                cli_avs_request.avs_params.rail_id,
                ((cli_avs_request.avs_response_single_resp) / 10),
                ((cli_avs_request.avs_response_single_resp) % 10));
            break;

        default:
            FpFwCliPrint(" AVS CLI default\n");
            break;
        }
        break;
    }
}

static FPFW_CLI_STATUS scp_avs_read_data_cli(int argc, const char** argv)
{
    FpFwCliPrint("\nscp_avs_read_data_cli func. call\n\n");

    if (argc == 4)
    {
        int avs_bus = atoi(argv[1]);
        if (avs_bus < 0 || avs_bus >= MAX_AVS_INST)
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

        cli_avs_interface->Device->avs_bus_num = avs_bus;
        cli_avs_request.avs_params.rail_id = rail_sel;
        cli_avs_request.avs_params.cmd_type = cmd_type;

        scp_avs_client_read(&cli_avs_interface->Header, &cli_avs_request.Header, AVSCLIRequestCompletion, NULL);
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

    if (argc == 5)
    {
        int avs_bus = atoi(argv[1]);
        if (avs_bus < 0 || avs_bus >= MAX_AVS_INST)
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

        cli_avs_interface->Device->avs_bus_num = avs_bus;
        cli_avs_request.avs_params.rail_id = rail_sel;
        cli_avs_request.avs_params.cmd_type = cmd_type;
        cli_avs_request.avs_params.data.avs_data = vr_data;

        scp_avs_client_write(&cli_avs_interface->Header, &cli_avs_request.Header, AVSCLIRequestCompletion, NULL);
    }
    else
    {
        FpFwCliPrint(" AVS write CLI Help\n");
        FpFwCliPrint("Cmds: 4, <avs_bus> <rail_sel> <cmd_type> <vr_data>\n");
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

void scp_avs_cli_initialize(pscp_avs_interface_t Interface)
{
    FpFwCliRegisterTable(scp_avs_cli_list, FPFW_ARRAY_SIZE(scp_avs_cli_list));

    cli_avs_interface = Interface;

    // This needs to be done for all AVS, so 4 interfaces.
    //  Perhaps pass pointers for the interface for each AVS bus. Can be a single config struct.
    DfwkClientInterfaceOpen(&Interface->Header); // goes into the AVS init. do this for all AVS so 4 interfaces

    //
    // Only need to initialize the async request once here.
    //
    DfwkAsyncRequestInititalize((PDFWK_ASYNC_REQUEST_HEADER)&cli_avs_request.Header, sizeof(cli_avs_request));
    FpFwCliPrint(" AVS CLI init complete\n");
}
