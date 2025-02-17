//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file system_info.h
 *  APIs for clients to get system information
 */

#pragma once

/*--------------- Includes ---------------*/
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <idsw.h>       // for idsw_get_platform_sdv,
#include <idsw_kng.h>   // for idsw_get_platform_sdv,
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 * Representation of the current security state of the HSP.
 */
enum hsp_security_state {
	HSP_SECURITY_STATE_UNKNOWN = 0x00,		/**< The security state could not be determined. */
	HSP_SECURITY_STATE_BLANK = 0x01,		/**< No security state has been set. */
	HSP_SECURITY_STATE_TEST = 0x02,			/**< The device is currently in Test state. */
	HSP_SECURITY_STATE_PRODUCTION = 0x04,	/**< The device is currently in Production state. */
	HSP_SECURITY_STATE_SECURE = 0x08,		/**< The device is currently in Secure state. */
	HSP_SECURITY_STATE_RETEST = 0x10,		/**< The device is currently in ReTest state. */
};

typedef enum hsp_security_state hsp_security_state_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief Checks if the HSP (Hardware Security Processor) is present.
 * @note This function should be called after system_info_init() is called.
 * @return true if the HSP is present, false otherwise.
 */
bool system_info_is_hsp_present();
bool system_info_is_warm_start();

/**
 * @brief Caches the system information.
 * 
 * @param icc_base_ctx The ICC base context.
 * 
 */
void system_info_init(fpfw_icc_base_ctx_t* icc_base_ctx);

/**
 * @brief Retrieves the platform ID.
 * 
 */
KNG_PLAT_ID system_info_get_platform(void);

/**
 * @brief Retrieves the current security state of the HSP.
 * 
 * @return The current security state of the HSP.
 */
hsp_security_state_t system_info_get_security_state(void);