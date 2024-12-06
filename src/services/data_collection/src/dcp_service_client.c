//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcp_service_client.c
 * In the DCP protocol, there are some messages that are handled by DCS and not one of the specific telemetry
 * clients. This file contains the handling of those messages.
 */

/*------------- Includes -----------------*/
#include "dcp_service_client_i.h"

#include <FpFwUtils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void dcp_svc_client_handle_trp_msg(p_trp_msg_t msg)
{
    FPFW_UNUSED(msg);
}