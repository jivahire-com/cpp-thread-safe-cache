//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file icc_cli_i.h
 * Private header for icc cli functionality.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <icc_cli.h>

/*-- Symbolic Constant Macros (defines) --*/
#define ICC_CLI_LOG_LEVEL          1

#define FPFW_CLI_LOG_ERR(fmt, ...) FpFwCliPrint(fmt "\n", ##__VA_ARGS__)
#define FPFW_CLI_LOG_INFO(fmt, ...) \
    if (ICC_CLI_LOG_LEVEL >= 1)     \
    FpFwCliPrint(fmt "\n", ##__VA_ARGS__)
#define FPFW_CLI_LOG_DEBUG(fmt, ...) \
    if (ICC_CLI_LOG_LEVEL >= 2)      \
    FpFwCliPrint(fmt "\n", ##__VA_ARGS__)
/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

icc_cli_init_params_t * get_icc_cli_ctx(void);
