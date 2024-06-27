//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_cli.h
 * Cli module for exposing icc commands
 */

#pragma once

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 * @brief List of transport interfaces supported by icc cli
 * 
 */
typedef enum {
    ICC_CLI_HSP_MBX = 0, //! local core is scp/mcp, remote is hsp
    ICC_CLI_SDM_FIFO_MBX, //! remote is sdm
    ICC_CLI_CDED_FIFO_MBX, //! remote is cded
    ICC_CLI_D2D_MBX,    //! tbd
    ICC_CLI_MSCP_MHU, //! if local is scp, remote mcp & vice versa
    ICC_CLI_AP_NS_MHU, //! local core is scp/mcp, remote is ap ns
    ICC_CLI_AP_S_MHU, 
    ICC_CLI_AP_RT_MHU, 
    ICC_CLI_AP_RL_MHU,
    ICC_CLI_MAX_TRANSPORT_TYPE,
} icc_cli_interface_type;

/**
 * @brief Parameters for initializing the icc_cli module
 * 
 */
typedef struct
{
    void *icc_base_ctx[ICC_CLI_MAX_TRANSPORT_TYPE];   //! ICC Base ctx for each Transport interface to use for the cli
} icc_cli_init_params_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief API to initialize the icc_cli module
 * 
 * @param params 
 */
void icc_cli_init(icc_cli_init_params_t *params);
