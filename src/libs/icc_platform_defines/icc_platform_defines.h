//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file icc_platform_defines.h
 *  Header for adding ICC related platform defines, macros, etc.
*/

#pragma once

/*--------------- Includes ---------------*/

/*---------------- Macros ----------------*/
#define HSP_MBOX_MAX_MESG_SIZE_BYTES            (16U)
#define LARGE_FIFO_MBOX_MAX_MESG_SIZE_BYTES     (128U)
#define D2D_FIFO_MBOX_MAX_MESG_SIZE_BYTES       (64U)

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
#define HSP_MBOX_CMD_CODE_SIZE       (16U)
#define HSP_MBOX_CMD_CODE_START_POS  (0U) //! 16th bit from lsb
#define HSP_MBOX_MAX_SEQ_NUM         (0xFFU)
#define HSP_MBOX_MIN_SEQ_NUM         (0x0U)
#define HSP_MBOX_SEQ_NUM_SIZE        (8U)
#define HSP_MBOX_SEQ_NUM_START_POS   (16U) //! 24th bit from lsb

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
#define D2D_MBOX_MIN_CMD_CODE        (0x1U)
#define D2D_MBOX_CMD_CODE_SIZE       (16U)
#define D2D_MBOX_CMD_CODE_START_POS  (0U) //! 16th bit from lsb
#define D2D_MBOX_MAX_SEQ_NUM         (0xFFU)
#define D2D_MBOX_MIN_SEQ_NUM         (0x0U)
#define D2D_MBOX_SEQ_NUM_SIZE        (8U)
#define D2D_MBOX_SEQ_NUM_START_POS   (16U) //! 24th bit from lsb

/*------------- Typedefs -----------------*/
