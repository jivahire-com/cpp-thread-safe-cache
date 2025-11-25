//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_advlog_pldm.h
 * AP adv logger pldm request processing for mcp core
 */

/*------------- Includes -----------------*/

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
/**
 * @brief Initialize the AP adv logger PLDM service on MCP, no effect on SCP.
 *
 */
void ap_advlog_pldm_init(void);

/**
 * @brief - Transfer the advanced log dump using a PLDM polled platform
 *          event to the BMC.
 *
 * @param None
 *
 * @return - None
 */
void ap_advlog_pldm_transfer_dump(void);
