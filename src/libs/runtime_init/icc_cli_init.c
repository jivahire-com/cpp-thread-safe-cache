/**
 * @file icc_cli_init.c
 * Initialize the icc cli library.
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <icc_cli.h>   // for icc_cli_init
#include <stddef.h>    // for NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(icc_cli, FPFW_INIT_DEPENDENCIES("cli", "icc_hspmbx"))
{
    //! set the available transport interfaces
    static icc_cli_init_params_t icc_cli_params;
    icc_cli_params.icc_base_ctx[ICC_CLI_HSP_MBX] = fpfw_init_get_handle("icc_hspmbx");
    icc_cli_params.icc_base_ctx[ICC_CLI_SDM_FIFO_MBX] = NULL;
    icc_cli_params.icc_base_ctx[ICC_CLI_CDED_FIFO_MBX] = NULL;
    icc_cli_params.icc_base_ctx[ICC_CLI_D2D_MBX] = NULL;
    icc_cli_params.icc_base_ctx[ICC_CLI_MSCP_MHU] = NULL;
    icc_cli_params.icc_base_ctx[ICC_CLI_AP_NS_MHU] = NULL;
    icc_cli_params.icc_base_ctx[ICC_CLI_AP_S_MHU] = NULL;
    icc_cli_params.icc_base_ctx[ICC_CLI_AP_RT_MHU] = NULL;
    icc_cli_params.icc_base_ctx[ICC_CLI_AP_RL_MHU] = NULL;

    icc_cli_init(&icc_cli_params);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
