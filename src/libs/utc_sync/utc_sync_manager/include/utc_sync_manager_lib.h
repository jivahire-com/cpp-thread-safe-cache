//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file utc_sync_manager_lib.h
 * This file contains the function prototypes and definitions for the UTC Sync Manager KNG library.
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

#include <fpfw_mctp.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Requests a new UTC timestamp from the BMC via MCTP.
 *
 * This function will communicate with the BMC in order to retrieve the current UTC time.
 * It sets up the necessary MCTP send and receive message structures,
 * pends a receive request, and sends the request message.
 * 
 * If either the send or receive request fails, an error event will be logged.
 *
 * Any failed receive request will be re-pended to ensure readiness for the next send.
 * Failed send requests will not be re-pended and they should be handled by the caller.
 * 
 * @return FPFW_STATUS_SUCCESS if the request was successfully initiated.
 *         FPFW_STATUS_FAIL or another appropriate error code if the request failed.
 */
fpfw_status_t utc_sync_manager_request_utc_timestamp(void);

/**
 * @brief Initializes the UTC Sync Manager MCTP library.
 *
 * @param mctp_ctx Pointer to the MCTP context. To be passed in from runtime_init.
 *
 * @return None
 */
void utc_sync_manager_mctp_init(fpfw_mctp* mctp_ctx);