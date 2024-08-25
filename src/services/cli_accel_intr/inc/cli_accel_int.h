//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cli_accel_int.h
 * Header file for accel_interrupt_handler CLI
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <FpFwCli.h>            // for FpFwCliDisplayUsage, FPFW_C...

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/
// Operation to execute on register
typedef enum {
    REG_READ = 0x0,
    REG_WRITE,
    REG_CLR,
    REG_SET
} REG_RW_OPERATION;

// Operations supported by a register
typedef enum {
    READ_WRITE = 0x0,
    READ_ONLY,
    READ_WRITE_ONE_TO_CLEAR,
    READ_WRITE_ONE_TO_SET
} REG_RW_TYPE;

// Register data
typedef struct {
    uint32_t reg_address;       // reg_address : Address of register
    REG_RW_TYPE reg_rw_type;    // reg_rw_type : Type of operation supported by the register
    uint32_t word_size;         // Number of bytes per word access.
} reg_data_t, *p_reg_data_t;

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
/**
 * @brief Initialise cli_accel_int
 *
 * \b Description:
 * Registers the commands executed by cli_accel_int with FpFw CLI
 *
 * @param[in]
 * void
 *
 * @retval
 *  On success, CLI_SUCCESS
 *  On failure, CLI_ERROR
 * */
FPFW_CLI_STATUS cli_accel_int_init(void);
