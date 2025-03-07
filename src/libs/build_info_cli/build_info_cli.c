//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file build_info_cli.c
 * Source file to implement cli for build_info
 */

/*-------------------------------- Includes ---------------------------------*/
#include <FpFwCli.h>        // for FPFW_CLI_STATUS, FpFwCliRegister, CLI_SU...
#include <FpFwLinkedList.h> // for NULL_LIST_ENTRY
#include <FpFwUtils.h>      // for FPFW_UNUSED
#include <build_data.h>     // for BUILD_PC, BUILD_TIMESTAMP, GIT_BRANCH
#include <stdio.h>          // for printf

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
static FPFW_CLI_STATUS whoami(int Argc, const char** Argv);

/*------------------- Declarations (Statics and globals) --------------------*/
static FPFW_CLI_COMMAND s_build_info_commands = {NULL_LIST_ENTRY, "build_info", "whoami", whoami, "show build metadata", "Usage: whoami"};

/*--------------------------------- Externs ---------------------------------*/

/*----------------------------- Static Functions ----------------------------*/
static FPFW_CLI_STATUS whoami(int Argc, const char** Argv)
{
    FPFW_UNUSED(Argc);
    FPFW_UNUSED(Argv);

    printf("----------------------------------------------------------------\n");
    printf("Build Version: %d.%d.%d\n", (int)MAJOR_VERSION, (int)MINOR_VERSION, (int)PATCH_VERSION);
    printf("Branch: %s\n", GIT_BRANCH);
    printf("Commit: %s\n", GIT_COMMIT_SHA);
    printf("Build Timestamp: %s\n", BUILD_TIMESTAMP);
    printf("Build PC: %s\n", BUILD_PC);
    printf("----------------------------------------------------------------\n");
    /* MSCP Build Info */
    printf("MSCP tag " SCP_DESCRIBE "\n");
    printf("MSCP Commit " GIT_COMMIT_SHA "\n");

    /* Silibs Info */
    printf("Silibs tag " SILIBS_DESCRIBE "\n");
    printf("Silibs Commit " SILIBS_LAST_COMMIT_DESCRIBE "\n");

    /* SDM Info */
    printf("SDM tag " SDM_DESCRIBE "\n");
    printf("SDM Commit " SDM_LAST_COMMIT_DESCRIBE "\n");

    printf("----------------------------------------------------------------\n");

    return CLI_SUCCESS;
}

/*----------------------------- Global Functions ----------------------------*/
FPFW_CLI_STATUS build_info_cli_init(void)
{
    FpFwCliRegister(&s_build_info_commands);

    return CLI_SUCCESS;
}
