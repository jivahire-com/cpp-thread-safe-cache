//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_lt_events.h
 * Implements the shared data/functions that are used for signalling that
 * link training sequence has begun. Primary use is to sequence primary AP core
 * boot such that it happens post link training.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <kng_soc_constants.h>
#include <startup_shutdown_ssi.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/*
 *  @brief      Caches the link training startup phase request which will be
 *              completed once ltssm_en is set (on the first enabled RPSS).
 *
 * @retval     None
 */
void cache_ssi_ltssm_startup_request(pssi_startup_notification_request_t ltssm_req);

/*
 * @brief      Completes the cached link training startup phase request.
 *
 * @param[in]  rpss_idx  RPSS index signaling the completion.
 *
 * @retval     None
 */
void complete_ssi_ltssm_startup_req(RPSS_INSTANCE rpss_idx);

/*
 * @brief      Checks if the link training sequence has been initiated on the
 *            first enabled RPSS.
 *
 * @retval     true if link training sequence has been initiated, false otherwise.
 */
bool scp_is_pcie_ltssm_en_set();
