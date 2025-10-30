//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ift_fw.h
 *  In Field Testing API prototypes and structure definitions
 */

#pragma once

/*----------- Nested includes ------------*/

#include <DfwkDriver.h>
#include <fpfw_icc_base.h>
#include <fpfw_status.h>
#include <kingsgate_hsp_mailbox_commands.h> // for HSP_MAILBOX_CMD_IFT_INTENT_REQ, HSP_MAILBOX_CMD_IFT_RUN_STATUS_REQ
#include <silibs_platform.h>

/*-- Symbolic Constant Macros (defines) --*/

#define IFT_STATUS_DISABLED 0
#define IFT_STATUS_ENABLED  1

#define IFT_CLK_DIV_A            5
#define IFT_CLK_DIV_B            0
#define IFT_CLK_DIV_TIMEOUT      0x12000

/*---------------- enums -----------------*/

enum 
{
    IFT_MGR_IFT_INTENT_MEM_STATE_CLEAR = 0,
    IFT_MGR_IFT_INTENT_FLOP_STATE_CLEAR = 1,
    IFT_MGR_IFT_INTENT_MEM_TEST = 2,
    IFT_MGR_IFT_INTENT_CORE_TEST = 3,
    IFT_MGR_IFT_INTENT_TYPE_MAX = 4,
};

/*-------------- Typedefs ----------------*/

typedef struct
{
    DFWK_DEVICE_HEADER Header;
    DFWK_QUEUE Queue;
} ift_device_t, *pift_device_t;

typedef enum
{
    IFT_RUN_MEM_TESTS_ASYNC,
    IFT_RUN_CORE_TESTS_ASYNC,
    IFT_DFWK_REQUEST_TYPE_MAX,
} IFT_DFWK_REQUEST_TYPE;

typedef struct
{
    DFWK_ASYNC_REQUEST_HEADER Header;
} ift_request_t, *pift_request_t;

typedef struct
{
    DFWK_INTERFACE_HEADER Header;
    pift_device_t Device;
} ift_interface_t, *pift_interface_t;


struct kng_hsp_mailbox_msg_req
{
    struct kng_hsp_mailbox_msg_header hdr;   /**< msg header containing cmd, seq, context and flags. */
    uint32_t status;                         /**< Status of the processed incoming mailbox messages. */
    uint32_t status_ex;                      /**< Additional status information or actions (e.g., goto recovery). */
};

union kng_hsp_mailbox_ift_intent_msg
{
    struct kng_hsp_mailbox_msg_header ift_intent_req;           /**< IFT intent request message header. */
    struct kng_hsp_mailbox_cmd_ift_intent_rsp ift_intent_rsp;   /**< IFT intent response structure. */
    uint32_t as_uint32[4];                                      /**< Raw access to message as 4 uint32 words. */
};

union kng_hsp_mailbox_ift_status_msg
{
    struct kng_hsp_mailbox_cmd_ift_run_status_req ift_status_req; /**< IFT run status request structure. */
    struct kng_hsp_mailbox_msg_header ift_status_rsp;             /**< IFT run status response message header. */
    uint32_t as_uint32[4];                                        /**< Raw access to message as 4 uint32 words. */
};

/*--------- Function Prototypes ----------*/

/**
 * @brief Initializes the In-Field Test (IFT) library and queries HSP for IFT status and intent.
 * Updates the MPU to make MSCP BOOT DATA in MSCP EXP RAM strongly ordered.
 * @param[in] hsp_icc_ctx Pointer to the HSP ICC context.
 */
void ift_init(fpfw_icc_base_ctx_t* hsp_icc_ctx);

/**
 * @brief Sets up and queues IFT tests for execution based on the current intent type.
 */
void ift_setup_tests(void);

/**
 * @brief Returns whether IFT is enabled.
 * @return true if IFT is enabled, false otherwise.
 */
bool ift_is_enabled(void);

/**
 * @brief Gets the current index of the IFT core test firmware.
 * @return Index of the core test firmware.
 */
uint16_t ift_get_core_test_fw_idx(void);

/**
 * @brief Initiates the IFT memory test sequence.
 * @param[in] request Pointer to the DFWK async request header.
 */
void ift_mem_test(PDFWK_ASYNC_REQUEST_HEADER request);

/**
 * @brief Initiates the IFT core test sequence.
 * @param[in] request Pointer to the DFWK async request header.
 */
void ift_core_test(PDFWK_ASYNC_REQUEST_HEADER request);

/**
 * @brief Initializes the IFT device for the driver framework.
 * @param[in] device Pointer to the IFT device structure.
 * @param[in] schedule Pointer to the DFWK schedule structure.
 */
void ift_dfwk_device_initialize(pift_device_t device, PDFWK_SCHEDULE schedule);

/**
 * @brief Initializes the IFT interface for the driver framework.
 * @param[in] intf Pointer to the IFT interface structure.
 * @param[in] device Pointer to the IFT device structure.
 */
void ift_dfwk_interface_initialize(pift_interface_t intf, pift_device_t device);

/**
 * @brief Sets the current IFT interface in the driver framework.
 * @param[in] p_interface Pointer to the IFT interface structure.
 */
void ift_dfwk_set_interface(pift_interface_t p_interface);

/**
 * @brief Gets the base address of the IFT firmware binary in memory.
 * @return Base address of the IFT firmware binary.
 */
uint32_t ift_get_ift_fw_addr(void);

/**
 * @brief Sets the current IFT firmware binary size.
 * @param[in] size Size of the firmware binary in bytes.
 */
void ift_set_current_fw_size(uint32_t size);
