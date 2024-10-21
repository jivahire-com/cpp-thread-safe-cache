/**
 * @file fuse_init.c
 * Instantiates fuse service
 */

/*------------- Includes -----------------*/
#include <bug_check.h>     // for BUG_ASSERT_PARAM, BUG_ASSERT
#include <fpfw_init.h>
#include <fuse_client.h>
#include <fuse_init.h>
#include <stdio.h>               // for NULL, printf
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(fuse_svc, FPFW_INIT_DEPENDENCIES("mesh","icc_hspmbx"))
{
    fpfw_icc_base_ctx_t* icc_hspmbx_ctx = fpfw_init_get_handle("icc_hspmbx");
    if (icc_hspmbx_ctx == NULL) {
    printf(FUSE_NAME "Error: Failed to get icc_hspmbx context\n");
    BUG_CHECK(FPFW_INIT_STATUS_E_CYCLE,icc_hspmbx_ctx,"icc_hspmbx");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_E_CYCLE, NULL};
   }
    fuse_init(icc_hspmbx_ctx);
    platform_fuse_override();
    platform_fuse_distribution(0);
    printf(FUSE_NAME "SVC init successfully\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cli_fuse, FPFW_INIT_DEPENDENCIES("cli", "fuse_svc","mesh"))
{
    FPFW_CLI_STATUS status = platform_fuse_init_cli();
    platform_fuse_distribution(1);
    return (fpfw_init_result_t){status, NULL};
}
