//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Implements a command line interface to interact with the pcie root port
 * management service on the SCP.
 */

/*------------- Includes -----------------*/
#include <DfwkClient.h>
#include <FpFwCli.h>
#include <kng_soc_constants.h>
#include <pcie_cli_i.h>
#include <pcie_dfwk.h>
#include <pcie_rp_common.h>
#include <pcie_ss_common.h>
#include <rc4sx16_pf0_type1_hdr_regs.h>
#include <silibs_status.h>
#include <stdint.h>
#include <stdlib.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static FPFW_CLI_STATUS dump_rpss_entity(int argc, const char** argv);
static FPFW_CLI_STATUS dump_rp_entity(int argc, const char** argv);
static FPFW_CLI_STATUS dump_rp_link_info(int argc, const char** argv);
static FPFW_CLI_STATUS dump_rp_dbi_cfg_hdr(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/
/* clang-format off */
static FPFW_CLI_COMMAND pcie_cli_table[] = {
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rpss_entity",
        .Routine = dump_rpss_entity,
        .Description = "Shows rpss entity characteristics",
        .Usage = "pcie dump_rpss_entity <rpss_number between 0-7>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rp_entity",
        .Routine = dump_rp_entity,
        .Description = "Shows rp entity characteristics",
        .Usage = "pcie dump_rp_entity <rpss_number between 0-7> <rp_number between 0-3>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rp_link_info",
        .Routine = dump_rp_link_info,
        .Description = "Shows link information for an enabled root port",
        .Usage = "pcie dump_rp_link_info <rpss_number between 0-7> <rp_number between 0-3>"
    },
    {
        .ListEntry = NULL_LIST_ENTRY,
        .MenuName = "pcie",
        .Syntax = "dump_rp_dbi_cfg_hdr",
        .Routine = dump_rp_dbi_cfg_hdr,
        .Description = "Shows the root port type 1 configuration header (over DBI) for an enabled root port",
        .Usage = "pcie dump_rp_dbi_cfg_hdr <rpss_number between 0-7> <rp_number between 0-3>"
    },
};
/* clang-format on */

static pciess_device_interface_t iface[PCIE_RPSS_PER_DIE] = {0};
static pcie_ss_entity_t* curr_ss_entity = NULL;

/*------------- Functions ----------------*/
static FPFW_CLI_STATUS dump_rpss_entity(int argc, const char** argv)
{
    if (argc != 2)
    {
        return CLI_ERROR;
    }

    RPSS_INSTANCE rpss_idx = (RPSS_INSTANCE)(strtoul(argv[1], NULL, 0));
    if (rpss_idx >= NUM_RPSS)
    {
        return CLI_ERROR;
    }

    curr_ss_entity = NULL;
    pcie_sync_request_t req = {0};
    req.p_requested_data = (void*)(&curr_ss_entity);
    req.rpss_index = rpss_idx;
    req.req_type = GET_RPSS_ENTITY_REQUEST;
    req.header.RequestType = GET_RPSS_ENTITY_REQUEST;
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx % PCIE_RPSS_PER_DIE]), &req.header);

    if (req.status == SILIBS_SUCCESS && curr_ss_entity != NULL)
    {
        print_rpss_entity(curr_ss_entity);
    }
    else
    {
        FpFwCliPrint("pcie cli: Failed to get entity from the PCIe driver! silibs_status: %d\n", (int)req.status);
        return CLI_ERROR;
    }
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS dump_rp_entity(int argc, const char** argv)
{
    if (argc != 3)
    {
        return CLI_ERROR;
    }

    RPSS_INSTANCE rpss_idx = (RPSS_INSTANCE)(strtoul(argv[1], NULL, 0));
    uint8_t rp_idx = (RPSS_INSTANCE)(strtoul(argv[2], NULL, 0));

    if (rpss_idx >= NUM_RPSS || rp_idx >= PCIESS_NUM_PORTS)
    {
        return CLI_ERROR;
    }

    curr_ss_entity = NULL;
    pcie_sync_request_t req = {0};
    req.p_requested_data = (void*)(&curr_ss_entity);
    req.rpss_index = rpss_idx;
    req.rp_index = rp_idx;
    req.req_type = GET_RPSS_ENTITY_REQUEST;
    req.header.RequestType = GET_RPSS_ENTITY_REQUEST;
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx % PCIE_RPSS_PER_DIE]), &req.header);

    if (req.status == SILIBS_SUCCESS && curr_ss_entity != NULL)
    {
        print_rp_entity(&(curr_ss_entity->rps[rp_idx]));
    }
    else
    {
        FpFwCliPrint("pcie cli: Failed to get entity from the PCIe driver! silibs_status: %d\n", (int)req.status);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS dump_rp_link_info(int argc, const char** argv)
{
    if (argc != 3)
    {
        return CLI_ERROR;
    }

    RPSS_INSTANCE rpss_idx = (RPSS_INSTANCE)(strtoul(argv[1], NULL, 0));
    uint8_t rp_idx = (RPSS_INSTANCE)(strtoul(argv[2], NULL, 0));

    if (rpss_idx >= NUM_RPSS || rp_idx >= PCIESS_NUM_PORTS)
    {
        return CLI_ERROR;
    }

    PCIE_LTSSM_STATE ltssm_state = 0;
    pcie_cli_req_op_t cli_op = GET_RP_LTSSM_STATE;
    pcie_link_state_t* current_link_state = {0};

    pcie_sync_request_t req = {0};
    req.rpss_index = rpss_idx;
    req.rp_index = rp_idx;
    req.req_type = CLI_REQUEST;
    req.header.RequestType = CLI_REQUEST;
    req.p_sent_data = (void*)(&cli_op);
    req.p_requested_data = (void*)(&ltssm_state);
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx % PCIE_RPSS_PER_DIE]), &req.header);
    if (req.status != SILIBS_SUCCESS)
    {
        FpFwCliPrint("pcie cli: Failed to get ltssm state from the PCIe driver! silibs_status: %d\n", (int)req.status);
        return CLI_ERROR;
    }

    cli_op = GET_RP_LINK_STATE;
    req.p_requested_data = (void*)(&current_link_state);
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx]), &req.header);

    print_link_state(rpss_idx, rp_idx, ltssm_state, current_link_state);

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS dump_rp_dbi_cfg_hdr(int argc, const char** argv)
{
    if (argc != 3)
    {
        return CLI_ERROR;
    }

    RPSS_INSTANCE rpss_idx = (RPSS_INSTANCE)(strtoul(argv[1], NULL, 0));
    uint8_t rp_idx = (RPSS_INSTANCE)(strtoul(argv[2], NULL, 0));

    if (rpss_idx >= NUM_RPSS || rp_idx >= PCIESS_NUM_PORTS)
    {
        return CLI_ERROR;
    }

    rc4sx16_pf0_type1_hdr_reg* rp_t1_hdr = NULL;
    pcie_cli_req_op_t cli_op = GET_RP_DBI_CONFIG_HDR;

    /* Setup and send the DFWK request */
    pcie_sync_request_t req = {0};
    req.rpss_index = rpss_idx;
    req.rp_index = rp_idx;
    req.req_type = CLI_REQUEST;
    req.header.RequestType = CLI_REQUEST;
    req.p_sent_data = (void*)(&cli_op);
    req.p_requested_data = (void*)(&rp_t1_hdr);
    DfwkInterfaceSendSync((PDFWK_INTERFACE_HEADER)(&iface[rpss_idx % PCIE_RPSS_PER_DIE]), &req.header);

    if (req.status == SILIBS_SUCCESS)
    {
        print_type1_rp_header(rpss_idx, rp_idx, rp_t1_hdr);
    }
    else
    {
        FpFwCliPrint("pcie cli: Failed to get root port config from the PCIe driver! silibs_status: %d\n",
                     (int)req.status);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

void pcie_cli_init(pciess_device_t* pcie_dev_handles)
{
    if (pcie_dev_handles == NULL)
    {
        return;
    }

    /* Initialize and open up PCIe interfaces so that we can issue requests to the PCIe driver later on */
    for (uint8_t i = 0; i < PCIE_RPSS_PER_DIE; i++)
    {
        pcie_dfwk_interface_init(&(pcie_dev_handles[i]), &(iface[i]));
        DfwkClientInterfaceOpen(&(iface[i].header));
    }

    FpFwCliRegisterTable(&pcie_cli_table[0], (sizeof(pcie_cli_table) / sizeof(FPFW_CLI_COMMAND)));
}
