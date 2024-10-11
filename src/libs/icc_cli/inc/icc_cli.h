//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_cli.h
 * Cli module for exposing icc commands
 */

#pragma once

/*----------- Nested includes ------------*/

#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 * @brief List of transport interfaces supported by icc cli
 * 
 */
typedef enum {
    ICC_CLI_HSP_MBX = 0, //! local core is scp/mcp, remote is hsp
    ICC_CLI_SDM_FIFO_MBX, //! local core is scp/mcp, remote is sdm
    ICC_CLI_CDED_FIFO_MBX, //! local core is scp/mcp, remote is cded
    ICC_CLI_D2D_MBX,    //! Die to die for only Scp over SPI bridge mailbox
    ICC_CLI_MSCP_MHU, //! if local is scp, remote mcp & vice versa
    ICC_CLI_AP_NS_MHU, //! local core is scp/mcp, remote is ap ns
    ICC_CLI_AP_S_MHU, //! local core is scp/mcp, remote is ap s
    ICC_CLI_AP_RT_MHU, //! local core is scp/mcp, remote is ap rt
    ICC_CLI_AP_RL_MHU, //! local core is scp/mcp, remote is ap rl
    ICC_CLI_D2D_MHU, //! Die to die for Scp or MCP over MHU IP
    ICC_CLI_MAX_TRANSPORT_TYPE,
} icc_cli_interface_type;

/**
 * @brief Provide current setup info in which the cli options will be invoked.
 * Depending on the combination of platform_id, die_id and core_id, some cli options
 * may or may not be available.
 */
typedef union _icc_cli_setup_info
{
    struct {
        uint32_t platform_id:8;
        uint32_t die_id:8;
        uint32_t core_id:8;
        uint32_t platform_is_multi_die:1;
        uint32_t reserved:7;
    }current;
    uint32_t as_uint32;
}icc_cli_setup_info;

/**
 * @brief Parameters for initializing the icc_cli module
 * 
 */
typedef struct
{
    void *icc_base_ctx[ICC_CLI_MAX_TRANSPORT_TYPE];   //! ICC Base ctx for each Transport interface to use for the cli
    icc_cli_setup_info setup_info; //! Setup info for the current core
} icc_cli_init_params_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief API to initialize the icc_cli module
 * 
 * @param params 
 */
void icc_cli_init(icc_cli_init_params_t *params);
