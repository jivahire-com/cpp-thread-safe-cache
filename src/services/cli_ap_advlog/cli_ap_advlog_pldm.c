//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_ap_advlog_pldm.c
 * Source file to implement cli for ap adv log
 */

/*-------------------------------- Includes ---------------------------------*/
#include <DbgPrint.h>
#include <FpFwCli.h>         // for FpFwCliPrint, FPFW_CLI_COM...
#include <FpFwLinkedList.h>  // for NULL_LIST_ENTRY
#include <FpFwUtils.h>       // for FPFW_ARRAY_SIZE
#include <ap_advlog_pldm.h>  // for ap_advlog_pldm_transfer_dump
#include <idsw.h>            // for idsw_get_platform_sdv,
#include <idsw_kng.h>        // for idsw_get_platform_sdv,
#include <silibs_platform.h> // for MMIO_WRITE32, MMIO_READ32
#include <stdint.h>          // for uint32_t, uint8_t
#include <stdlib.h>          // for errno, strtol
#include <utils.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
static FPFW_CLI_STATUS cli_ap_advlog(int argc, const char** argv);

/*------------------- Declarations (Statics and globals) --------------------*/
// clang-format off
static FPFW_CLI_COMMAND s_ap_advlog_int_commands_table[] = {
    {NULL_LIST_ENTRY, "ap_advlog", "dump_advlog",  cli_ap_advlog,  "Dump Ap Adv Logs to BMC", "Usage: dump_advlog"},
};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/

/**
 * @brief Routine to read platform ID
 *
 * \b Description:
 * Reads the platform.
 *
 * @retval
 *  On success, CLI_SUCCESS
 *  On failure, CLI_ERROR
 * */
static PLACED_CODE FPFW_CLI_STATUS cli_ap_advlog(int argc, const char** argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(argv);

    FPFW_DBGPRINT_INFO("Ap Adv Logger Cli\n");

    ap_advlog_pldm_transfer_dump();

    return CLI_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/
PLACED_CODE FPFW_CLI_STATUS cli_ap_advlog_pldm_init(void)
{
    FpFwCliRegisterTable(s_ap_advlog_int_commands_table, FPFW_ARRAY_SIZE(s_ap_advlog_int_commands_table));
    FPFW_DBGPRINT_INFO("CLI AP Adv Logger PLDM initialized\n");

    return CLI_SUCCESS;
}
