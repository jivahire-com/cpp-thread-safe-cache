/**
 * @file fuse_init.c
 * Instantiates fuse service
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <fuse_client.h>
#include <fuse_init.h>
#include <stdio.h>
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(fuse_svc, FPFW_INIT_DEPENDENCIES("icc_hspmbx"))
{
    fpfw_icc_base_ctx_t* icc_hspmbx_ctx = fpfw_init_get_handle("icc_hspmbx");
    fuse_init(icc_hspmbx_ctx);
    platform_fuse_override();
    platform_fuse_distribution(0);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cli_fuse, FPFW_INIT_DEPENDENCIES("cli", "fuse_svc","mesh"))
{
    FPFW_CLI_STATUS status = platform_fuse_init_cli();
    platform_fuse_distribution(1);
    return (fpfw_init_result_t){status, NULL};
}
