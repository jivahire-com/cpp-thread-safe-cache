//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file build_info_cli_init.c
 * Instantiates build_info_cli for SCP
 */

/*-------------------------------- Includes ---------------------------------*/
#include <FpFwCli.h>         // for FPFW_CLI_STATUS
#include <build_info_cli.h>  // for build_info_cli_init
#include <fpfw_init.h>       // for FPFW_INIT_COMPONENT, FPFW_INIT_DEPENDENCIES
#include <stddef.h>          // for NULL

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*----------------------------- Static Functions ----------------------------*/

/*----------------------------- Global Functions ----------------------------*/
FPFW_INIT_COMPONENT(build_info_cli, FPFW_INIT_DEPENDENCIES("cli"))
{
    FPFW_CLI_STATUS status = build_info_cli_init();
    return (fpfw_init_result_t){status, NULL};
}