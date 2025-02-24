//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hsp_interaction_i.h
 * This is a header filer for HSP interaction internal interfaces and not be invoked in global context.
 */

/*----------- Nested includes ------------*/
#include <boot_status.h>
#include <stdbool.h>
#include <stdint.h>


#pragma once

/*-------------- Typedefs ----------------*/


/*--------- Function Prototypes ----------*/
/**
 *  @brief This function keeps CPU wait for milliseconds requested hence acting as sleep.
 *
 *  @note This is approximation done based on CPU frequency and not be held against accuracy
 * 
 *  @param[in] milliseconds for sleep
 *
 *  @return
 *      None
 */
void sleep_ms(uint32_t millisecond);


/**
 *  @brief This function attempts to send boot status code of SCP/MCP to HSP over mailbox.
 *
 * 
 *  @param[in] boot_status_code
 *        [in] Is it SCP or MCP
 *        [in] Is status code fatal or non-fatal
 *
 *  @return
 *      returns true if successful in mailbox send or false if failed
 */
bool send_post_code(boot_status_code_t boot_post_code, bool is_scp, bool is_fatal);

/**
 *  @brief This function initialises HSP mailbox comunication for SCP/MCP based on address configured.
 *
 * 
 *  @param[in] SCP or MCP mailbox address
 *
 *  @return
 *      returns true if mailbox configure is success or false if failed
 */
bool hsp_mailbox_init(uint32_t mail_box_address);

