//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <DfwkCommon.h>
#include <scp_pcie_manager.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 *  @brief Worker thread for each root port subsystem.
 *
 *  @param[in]  thread_input  Thread context for each root port worker passed in
 *                            by ThreadX.
 *
 *  @return     None          NOTE: This thread never stop during SCP lifetime.
 */
void rpss_service_thread_fn(ULONG thread_input);

/**
 *  @brief Worker thread for setting the root bridge and VAB config variable.
 *
 *  @param[in]  thread_input  Thread context for each root port worker passed in
 *                            by ThreadX.
 *
 *  @return     None
 */
void config_variable_service_thread_fn(ULONG thread_input);

/**
 *  @brief Request the PCIe driver to initiate link training on all enabled root
 *         ports on a given root port subsystem.
 *
 *  @param[in]  ctx  Context associated with the root port subsystem to begin
 *                   link training on.
 *
 *  @return     None
 */
void send_start_link_training_requests(pcie_manager_context_t* ctx);

/**
 *  @brief Completion routine for request completion callbacks issued by the
 *         PCIe driver.
 *
 *  @param[in]  thread_input  Thread context for each root port worker passed in
 *                            by ThreadX.
 *
 *  @return     None          NOTE: This thread never stop during SCP lifetime.
 */
void rpss_req_completion_cb(PDFWK_ASYNC_REQUEST_HEADER req, void* ctx);
