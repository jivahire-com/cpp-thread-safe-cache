//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file cli_crash_dump_init.c
 * Instantiates CLI for Crash Dump
 */

/*------------- Includes -----------------*/
#include <crash_dump.h> // for crash_dump_cli_init
#include <fpfw_init.h>  // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <stddef.h>     // for NULL
#include <utils.h>

/*------- Symbolic Constant Macros (defines) ----------*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
PLACED_CODE FPFW_INIT_COMPONENT(cli_cd, FPFW_INIT_DEPENDENCIES("cli", "cd_init"))
{
    crash_dump_cli_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
