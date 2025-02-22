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
#include <idsw.h>          // for idsw_get_platform_sdv,
#include <idsw_kng.h>      // for idsw_get_platform_sdv,
#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/**
 * The macros below help parse the board id passed by HSP.
 *
 * The board id on Echo Falls is a 7-bit value which is divided into 3 parts:
 * Bit[0-2] - Rework revision
 * Bit[3-5] - Board design phase - RVP/RFU/EV/DV/PV
 * Bit[6]   - SoC position on dual socket boards 0/1
 */
#define BOARD_ID_GET_REWORK_REV(board_id)   ((board_id & 0x07))
#define BOARD_ID_GET_DESIGN_PHASE(board_id) ((board_id & 0x38) >> 3)
#define BOARD_ID_GET_SOC_POSITION(board_id) ((board_id & 0x40) >> 6)

/*-------------- Typedefs ----------------*/
/**
 * Representation of the current security state of the HSP.
 */
enum hsp_security_state
{
    HSP_SECURITY_STATE_UNKNOWN = 0x00,    /**< The security state could not be determined. */
    HSP_SECURITY_STATE_BLANK = 0x01,      /**< No security state has been set. */
    HSP_SECURITY_STATE_TEST = 0x02,       /**< The device is currently in Test state. */
    HSP_SECURITY_STATE_PRODUCTION = 0x04, /**< The device is currently in Production state. */
    HSP_SECURITY_STATE_SECURE = 0x08,     /**< The device is currently in Secure state. */
    HSP_SECURITY_STATE_RETEST = 0x10,     /**< The device is currently in ReTest state. */
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

/**
 * @brief Retrieves the board ID. The board ID is shared by HSP as part
 *        of boot metadata.
 *
 * @return The 7-bit board ID read from the platform.
 */
uint8_t system_info_get_board_id();
