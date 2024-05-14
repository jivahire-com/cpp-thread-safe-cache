//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc commands for hsp mbx.c
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>             // for FpFwCliPrint, _FPFW_CLI_STATUS, FPF...
#include <FpFwLinkedList.h>      // for NULL_LIST_ENTRY
#include <FpFwUtils.h>           // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <icc_cli.h>             // for icc_cli_init
#include <silibs_mcp_top_regs.h> // for MCP_TOP_MCP2HSP_MAILBOX_ADDRESS
#include <silibs_scp_top_regs.h> // for SCP_TOP_SCP2HSP_MAILBOX_ADDRESS
#include <stdint.h>              // for uint32_t, uint8_t
#include <stdlib.h>              // for atoi

/*-- Symbolic Constant Macros (defines) --*/
//! @todo Get the mailbox registers from the header files
#define MAX_ICC_MAILBOXES_INST 2
#define MAX_ICC_MAILBOX_REGS   8

/*-------------- Typedefs ----------------*/
typedef struct _icc_mbx_ctx_t
{
    uint32_t mbx_base_address;
    uint8_t mbx_name[16];
} icc_mbx_ctx_t, *p_icc_mbx_ctx_t;

static icc_mbx_ctx_t s_mbx_types[MAX_ICC_MAILBOXES_INST] = {
    {SCP_TOP_SCP2HSP_MAILBOX_ADDRESS, "SCP2HSP"},
    {MCP_TOP_MCP2HSP_MAILBOX_ADDRESS, "MCP2HSP"},
};

static const char* mbx_reg_names[MAX_ICC_MAILBOX_REGS] = {
    "S2H_CTRL",
    "S2H_INSTS",
    "S2H_FIFO_PUSH",
    "S2H_FIFO_POP",
    "H2S_CTRL",
    "H2S_INSTS",
    "H2S_FIFO_PUSH",
    "H2S_FIFO_POP",
};
/*--------- Function Prototypes ----------*/

static FPFW_CLI_STATUS display_mbx_list(int argc, const char** argv);
static FPFW_CLI_STATUS display_mbx_register_status(int argc, const char** argv);
static FPFW_CLI_STATUS set_mbx_reg_val(int argc, const char** argv);

/*-- Declarations (Statics and globals) --*/

static FPFW_CLI_COMMAND s_icc_cmd_list[] = {
    {NULL_LIST_ENTRY, "icc", "mbx_list", display_mbx_list, "Displays the mailbox instances", "Usage: mbx_list (no arguments)"},
    {NULL_LIST_ENTRY, "icc", "mbx_reg_show", display_mbx_register_status, "Displays the mailbox register status", "Usage: mbx_reg_show <inst_id, SCP=0, MCP=1>"},
    {NULL_LIST_ENTRY, "icc", "mbx_reg_set", set_mbx_reg_val, "Sets the mailbox register value", "Usage: mbx_reg_set <inst_id (SCP=0, MCP=1)> <reg_id(0 to 7)> <val(uint32_t)>"},
};

/*------------- Functions ----------------*/

void icc_cli_init(void)
{
    FpFwCliRegisterTable(s_icc_cmd_list, FPFW_ARRAY_SIZE(s_icc_cmd_list));
}

static FPFW_CLI_STATUS display_mbx_list(int argc, const char** argv)
{
    FPFW_UNUSED(argv);
    if (argc != 1)
    {
        return CLI_ERROR;
    }

    FpFwCliPrint("ICC Mailbox instances:\n");
    for (uint32_t i = 0; i < MAX_ICC_MAILBOXES_INST; i++)
    {
        FpFwCliPrint("  %d: %s (Address: 0x%08x)\n", i, s_mbx_types[i].mbx_name, s_mbx_types[i].mbx_base_address);
    }
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS display_mbx_register_status(int argc, const char** argv)
{
    FPFW_UNUSED(argv);
    if (argc != 2)
    {
        FpFwCliPrint("ERROR! Insufficient Args\n");
        return CLI_ERROR;
    }
    int inst_id = atoi(argv[1]);
    if (inst_id < 0 || inst_id >= MAX_ICC_MAILBOXES_INST)
    {
        FpFwCliPrint("ERROR! Invalid Arg\n");
        return CLI_ERROR;
    }

    uint32_t* mbx_address = (uint32_t*)s_mbx_types[inst_id].mbx_base_address;
    FpFwCliPrint("Mailbox instance %d: %s (Address: 0x%08x)\n",
                 inst_id,
                 s_mbx_types[inst_id].mbx_name,
                 s_mbx_types[inst_id].mbx_base_address);
    FpFwCliPrint("| Register       | Address(hex) | Value(hex) |\n");
    FpFwCliPrint("|----------------|--------------|------------|\n");
    for (uint32_t i = 0; i < MAX_ICC_MAILBOX_REGS; i++)
    {
        uint32_t reg_value = *(mbx_address + i);
        FpFwCliPrint("| %-14s | 0x%08x   | 0x%08x |\n", mbx_reg_names[i], (uint32_t)(mbx_address + i), reg_value);
    }
    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS set_mbx_reg_val(int argc, const char** argv)
{
    uint32_t old_value = 0;
    uint32_t new_value = 0;

    FPFW_UNUSED(argv);
    if (argc != 4)
    {
        FpFwCliPrint("ERROR! Insufficient Args\n");
        FpFwCliPrint("%s\n", s_icc_cmd_list[2].Usage);
        return CLI_ERROR;
    }
    int inst_id = atoi(argv[1]);
    int reg_id = atoi(argv[2]);
    int value = atoi(argv[3]);
    if (inst_id < 0 || inst_id >= MAX_ICC_MAILBOXES_INST || reg_id < 0 || reg_id >= MAX_ICC_MAILBOX_REGS)
    {
        FpFwCliPrint("ERROR! Invalid Arg\n");
        FpFwCliPrint("%s\n", s_icc_cmd_list[2].Usage);
        return CLI_ERROR;
    }

    uint32_t* mbx_address = (uint32_t*)s_mbx_types[inst_id].mbx_base_address;
    uint32_t* reg_address = mbx_address + reg_id;
    old_value = *reg_address;
    *reg_address = value;
    new_value = *reg_address;
    FpFwCliPrint("Mailbox inst %d:\tReg %s\tAddress: 0x%08x\tOld Val: 0x%x\tNew Val: 0x%x\n",
                 inst_id,
                 mbx_reg_names[reg_id],
                 (uint32_t)reg_address,
                 old_value,
                 new_value);
    return CLI_SUCCESS;
}