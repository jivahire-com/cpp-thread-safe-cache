//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file system_info_cli_init.c
 * Instantiates sys_info_cli for SCP
 */

/*-------------------------------- Includes ---------------------------------*/
#include <FpFwCli.h>         // for FPFW_CLI_STATUS
#include <cli_sys_info.h>
#include <fpfw_init.h>       // for FPFW_INIT_COMPONENT, FPFW_INIT_DEPENDENCIES
#include <stddef.h>          // for NULL

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
FPFW_INIT_COMPONENT(sys_info_cli, FPFW_INIT_DEPENDENCIES("cli", "hw_ver"))
{
    FPFW_CLI_STATUS status = cli_sys_info_init();
    return (fpfw_init_result_t){status, NULL};
}

