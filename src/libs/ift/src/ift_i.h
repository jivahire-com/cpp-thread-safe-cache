//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ift_i.h
 *  Private header for IFT library internal functions
 */

#pragma once

/*----------- Nested includes ------------*/

#include "ift_fw.h"

#include <fuses_top_regs.h>           // for FUSES_TOP_FUSE_RAM_SPACE_ADDRESS
#include <mscp_exp_rmss_memory_map.h> // for SCP_EXP_MSCP_BOOT_DATA_BASE
#include <scp_exp_top_regs.h>         // for SCP_EXP_TOP_SPI_CTRL_ADDRESS
#include <scp_top_regs.h>             // for SCP_TOP_DBGR_CSR_ADDRESS, SCP_TOP_SCP_EXP_ADDRESS
#include <stdio.h>

/*---------------- Defines ----------------*/

#define IFT_TEST_STATUS_SUCCESS    0x00 /**< IFT test success status */
#define IFT_TEST_STATUS_FAILURE    0x01 /**< IFT test failure status */
#define NUM_IFT_CORE_TEST_FW_COUNT 41   /**< Number of IFT core test firmware's */
#define NUM_IFT_MEM_TEST_FW_COUNT  2    /**< Number of IFT memory test firmware's */

#define SCP_TOP_DBGR_CSR_DBGR_IFT_OFFSET   0xF000
#define SCP_TOP_DBGR_IFT_ADDRESS           (SCP_TOP_DBGR_CSR_ADDRESS + SCP_TOP_DBGR_CSR_DBGR_IFT_OFFSET)

#define IFT_PATTERN_VERSION                0x5
#define IFT_WAIT_FOR_SCP_DIE1_US           (1000000)

#define SCP_EXP_IFT_BIN_DATA_BASE       (SCP_EXP_MSCP_BOOT_DATA_BASE)
#define SCP_EXP_IFT_BIN_DATA_MAX_SIZE   (512 * SL_1KB)
#define SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR (SCP_EXP_IFT_BIN_DATA_BASE + SCP_EXP_IFT_BIN_DATA_MAX_SIZE)
#define SCP_EXP_IFT_SYNC_MAGIC_NUM_SIZE (1 * SL_1KB)
#define SCP_EXP_IFT_SYNC_MAGIC_NUM      (0x1F77E57)
#define SCP_EXP_IFT_RESULT_BASE         (SCP_EXP_IFT_SYNC_MAGIC_NUM_ADDR + SCP_EXP_IFT_SYNC_MAGIC_NUM_SIZE)
#define SCP_EXP_IFT_RESULT_SIZE         (128 * SL_1KB)
/* One result contains two words (32-bit) */
#define SCP_EXP_IFT_RESULT_MAX          ((SCP_EXP_IFT_RESULT_SIZE / BYTES_IN_WORD32) >> 1)

#define SPI_CTRL_MASTER_REG (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SPI_CTRL_ADDRESS)

#define SYSTEM_FUSE_RAM_BASE_ADDR (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_FUSE_ADDRESS + FUSES_TOP_FUSE_RAM_SPACE_ADDRESS)

#define CORE_DEFECT_MFG_MASK_ARRAY_SIZE 3
#define HNS_DEFECT_MFG_MASK_ARRAY_SIZE 2

/*--------- Private Function Prototypes ----------*/

/**
 * @brief Send an asynchronous IFT request
 * @param request Pointer to the IFT request structure
 * @param request_type Type of IFT request to send
 */
void ift_dfwk_send_async_req(ift_request_t* request, uint32_t request_type);

/**
 * @brief Gets the current IFT firmware binary size.
 * @return Size of the firmware binary in bytes.
 */
uint32_t ift_get_current_fw_size(void);
/**
 * @brief Gets the current IFT intent type.
 * @return IFT intent type value.
 */
uint32_t ift_get_intent_type(void);

/**
 * @brief Queries HSP for IFT enabled status and intent type via ICC mailbox.
 * @param[in] hsp_icc_ctx Pointer to the HSP ICC context.
 * @param[out] ift_enabled Pointer to store IFT enabled status.
 * @param[out] ift_intent_type Pointer to store IFT intent type.
 */
void ift_icc_get_ift_intent(fpfw_icc_base_ctx_t* hsp_icc_ctx, uint8_t *ift_enabled, uint32_t *ift_intent_type);

/**
 * @brief Registers a callback for die-to-die firmware transfer completion.
 */
void ift_icc_register_d2d_fw_done();

/**
 * @brief Sends a die-to-die firmware transfer completion message via ICC mailbox.
 */
void ift_icc_send_d2d_fw_done_msg();

/**
 * @brief Sends IFT test results and status to HSP via ICC mailbox.
 * @param[in] hsp_icc_ctx Pointer to the HSP ICC context.
 */
void ift_icc_send_test_result_and_status(fpfw_icc_base_ctx_t* hsp_icc_ctx);

/**
 * @brief Sends a SOS request to load IFT firmware binaries
 * @param[in] request Pointer to the DFWK async request header.
 */
void ift_load_fw_sos(PDFWK_ASYNC_REQUEST_HEADER request);

/**
 * @brief Executes the IFT test sequence.
 * 
 * @param request Pointer to the DFWK async request header.
 */
void ift_execute_test(PDFWK_ASYNC_REQUEST_HEADER request);

/**
 * @brief Send request to execute IFT test via DFWK
 * 
 */
void ift_execute_test_dfwk(void);

/**
 * @brief Notifies SCP Die 0 that the IFT test has completed on SCP Die 1.
 * 
 */
void ift_notify_scp_die0();
