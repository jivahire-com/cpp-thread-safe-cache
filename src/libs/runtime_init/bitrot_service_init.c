//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file bitrot_service_init.c
 * This file contains the config/initialization for the Bitrot service
 */

/*------------- Includes -----------------*/

#include <bitrot_service.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>

// For memory address definitions
#define __NO_CSR_TYPEDEFS__
#define __NO_ADDRMAP_TYPEDEFS__
#include <mcp_exp_top_regs.h>
#include <mcp_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

static mscp_mem_table_t mscp_mem_table[] = {
    {.start_addr = (volatile mscp_mem_type*)MCP_TOP_MCP_INST_RAM_ADDRESS, .p_mem_type_name = "ITCM", .len = MCP_TOP_MCP_INST_RAM_SIZE},
    {.start_addr = (volatile mscp_mem_type*)MCP_TOP_MCP_DATA_RAM_ADDRESS, .p_mem_type_name = "DTCM", .len = MCP_TOP_MCP_DATA_RAM_SIZE},
    {.start_addr = (volatile mscp_mem_type*)MCP_TOP_MCP_DATA_RAM_ADDRESS, .p_mem_type_name = "DATA RAM", .len = MCP_TOP_MCP_DATA_RAM_SIZE},
};

/*------------------- Declarations (Statics and globals) --------------------*/
static mscp_bitrotservice_ctx_t ctx = {
    .mem_table_len = 3,
    .mem_table = mscp_mem_table,
};
/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(brt_svc, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver"))
{
    bitrot_thread_init(&ctx);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
