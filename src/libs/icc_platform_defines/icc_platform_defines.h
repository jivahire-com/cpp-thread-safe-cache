//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file icc_platform_defines.h
 *  Header for adding ICC related platform defines, macros, etc.
 *  
 *  @note The HSP and D2D mailbox message format is kept the same as dictated by the HSP firmware headers.
*/

#pragma once

/*--------------- Includes ---------------*/

/*---------------- Macros ----------------*/
#define HSP_MBOX_MAX_MESG_SIZE_BYTES            (16U)
#define LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES     (128U)
#define D2D_FIFO_MBOX_MAX_MESG_SIZE_BYTES       (64U)
#define D2D_MBOX_FIFO_DEPTH        (16U)

/**
 * HSP Mailbox Message Format (16 bytes):
 * 0    flag(4 bits)   ctx(4 bits)  seq(8 bits)  cmd(16 bits)
 * 1    data(32 bits)
 * 2    data(32 bits)
 * 3    data(32 bits)
 */
#define KG_HSP_MBOX_POLL_INTERVAL_NS (20000000ULL)
#define HSP_MBOX_MAX_CMD_CODE        (0xFFFFU)
#define HSP_MBOX_MIN_CMD_CODE        (0x1U)
#define HSP_MBOX_CMD_CODE_SIZE       (16U) //! 16 bits
#define HSP_MBOX_CMD_CODE_START_POS  (0U)  //! 16th bit from lsb
#define HSP_MBOX_MAX_SEQ_NUM         (0xFFU)
#define HSP_MBOX_MIN_SEQ_NUM         (0x0U)
#define HSP_MBOX_SEQ_NUM_SIZE        (8U)  //! 8 bits
#define HSP_MBOX_SEQ_NUM_START_POS   (16U) //! 24th bit from lsb
#define HSP_MBOX_FLAG_SIZE           (4U)  //! 4 bits
#define HSP_MBOX_FLAG_START_POS      (28U) //! 4th bit from lsb
#define HSP_MBOX_CONTEXT_SIZE        (4U)  //! 4 bits
#define HSP_MBOX_CONTEXT_START_POS   (24U) //! 8th bit from lsb

#define GET_HSP_MBOX_CMD_CODE(header)    (((header) >> HSP_MBOX_CMD_CODE_START_POS) & ((1U << HSP_MBOX_CMD_CODE_SIZE) - 1))
#define GET_HSP_MBOX_SEQ_NUM(header)     (((header) >> HSP_MBOX_SEQ_NUM_START_POS) & ((1U << HSP_MBOX_SEQ_NUM_SIZE) - 1))
#define GET_HSP_MBOX_CONTEXT(header)     (((header) >> HSP_MBOX_CONTEXT_START_POS) & ((1U << HSP_MBOX_CONTEXT_SIZE) - 1))
#define GET_HSP_MBOX_FLAGS(header)       (((header) >> HSP_MBOX_FLAG_START_POS) & ((1U << HSP_MBOX_FLAG_SIZE) - 1))

#define SET_HSP_MAILBOX_HEADER_ASUNIT32(cmd, ctx, flags) \
    ((((uint32_t)(flags) & ((1U << HSP_MBOX_FLAG_SIZE) - 1)) << HSP_MBOX_FLAG_START_POS)  | \
     (((uint32_t)(ctx) & ((1U << HSP_MBOX_CONTEXT_SIZE) - 1)) << HSP_MBOX_CONTEXT_START_POS) | \
     (((uint32_t)(cmd) & ((1U << HSP_MBOX_CMD_CODE_SIZE) - 1)) << HSP_MBOX_CMD_CODE_START_POS))

/**
 * D2D Mailbox Message Format (64 bytes): 4 bytes metadata + 60 bytes payload
 * 0    flag(4 bits)   ctx(4 bits)  seq(8 bits)  cmd(16 bits)
 * 1    data(32 bits)
 * 2    data(32 bits)
 * 3    data(32 bits)
 * ...
 * 15   data(32 bits)
 */
#define KG_D2D_MBOX_POLL_INTERVAL_NS (20000000ULL)
#define D2D_MBOX_MAX_CMD_CODE        (0xFFFFU)
#define D2D_MBOX_MIN_CMD_CODE        (0x0U)
#define D2D_MBOX_CMD_CODE_SIZE       (16U)
#define D2D_MBOX_CMD_CODE_START_POS  (0U) //! 16th bit from lsb
#define D2D_MBOX_MAX_SEQ_NUM         (0xFFU)
#define D2D_MBOX_MIN_SEQ_NUM         (0x0U)
#define D2D_MBOX_SEQ_NUM_SIZE        (8U)
#define D2D_MBOX_SEQ_NUM_START_POS   (16U) //! 24th bit from lsb
#define D2D_MBOX_FLAG_SIZE           (4U)  //! 4 bits
#define D2D_MBOX_FLAG_START_POS      (28U) //! 4th bit from lsb
#define D2D_MBOX_CONTEXT_SIZE        (4U)  //! 4 bits
#define D2D_MBOX_CONTEXT_START_POS   (24U) //! 8th bit from lsb

#define GET_RMSS_D2D_MBOX_CMD_CODE(header)    (((header) >> D2D_MBOX_CMD_CODE_START_POS) & ((1U << D2D_MBOX_CMD_CODE_SIZE) - 1))
#define GET_RMSS_D2D_MBOX_SEQ_NUM(header)     (((header) >> D2D_MBOX_SEQ_NUM_START_POS) & ((1U << D2D_MBOX_SEQ_NUM_SIZE) - 1))
#define GET_RMSS_D2D_MBOX_CONTEXT(header)     (((header) >> D2D_MBOX_CONTEXT_START_POS) & ((1U << D2D_MBOX_CONTEXT_SIZE) - 1))
#define GET_RMSS_D2D_MBOX_FLAGS(header)       (((header) >> D2D_MBOX_FLAG_START_POS) & ((1U << D2D_MBOX_FLAG_SIZE) - 1))

#define SET_RMSS_D2D_MAILBOX_HEADER_ASUNIT32(cmd, ctx, flags) \
        ((((uint32_t)(flags) & ((1U << D2D_MBOX_FLAG_SIZE) - 1)) << D2D_MBOX_FLAG_START_POS)  | \
     (((uint32_t)(ctx) & ((1U << D2D_MBOX_CONTEXT_SIZE) - 1)) << D2D_MBOX_CONTEXT_START_POS) | \
     (((uint32_t)(cmd) & ((1U << D2D_MBOX_CMD_CODE_SIZE) - 1)) << D2D_MBOX_CMD_CODE_START_POS))

/*------------- Typedefs -----------------*/
/**
 * @brief  RMSS D2D Mailbox response status
 * 
 */
typedef enum _rmss_d2d_mailbox_rsp_status {
	RMSS_D2D_MAILBOX_RSP_STATUS_SUCCESS = 0,
	RMSS_D2D_MAILBOX_RSP_STATUS_FAILURE = 1,
	RMSS_D2D_MAILBOX_RSP_STATUS_MAX
} rmss_d2d_mailbox_rsp_status;

/**
 * @brief  RMSS D2D Mailbox message command code table
 * 
 */
typedef enum _rmss_d2d_mailbox_msg_command_code {
    RMSS_D2D_MAILBOX_MSG_ECHO_REQ = 0x0,							
    RMSS_D2D_MAILBOX_MSG_ECHO_RSP,							
    RMSS_D2D_MAILBOX_MSG_MAX
}rmss_d2d_mailbox_msg_command_code;

/**
 * Generic struct used to parse incoming messages for the header elements (command code, sequence,context,flags)
 * Specialized commands will append to this structure.
 */
typedef struct _rmss_d2d_mailbox_msg_header {
	uint32_t cmd:16;	/**< request command. */
	uint32_t seq:8;		/**< request sequence. */
	uint32_t context:4;	/**< optional context field set at the discretion of the sender in a request. The receiver will copy the context to any associated response. */
	uint32_t flags:4;	/**< optional flags field specific to each mailbox request/response pair. */
}rmss_d2d_mailbox_msg_header;

/**
 * Below struct will be the response to requests received.
 */
typedef struct _rmss_d2d_mailbox_msg_rsp {
	rmss_d2d_mailbox_msg_header header;	/**< msg header conataining cmd,seq ,context and flags. */
	rmss_d2d_mailbox_rsp_status status;							/**< status of the processed incoming mailbox messages. */
	uint32_t status_ex;							/**< Optional, status additional information like error code. */
}rmss_d2d_mailbox_msg_rsp;

/**
 * @brief Generic D2D Mailbox message
 * 
 */
typedef union _rmss_d2d_mailbox_msg {
	rmss_d2d_mailbox_msg_header header;	/**< incoming mailbox message from protocol to handler. */
	rmss_d2d_mailbox_msg_rsp rsp;	        /**< outgoing mailbox message from handler to protocol. */
    uint32_t as_uint32[D2D_MBOX_FIFO_DEPTH];
} rmss_d2d_mailbox_msg;





