//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_handler.h
 * Public functions for error handler mock.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <stdbool.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 *  @brief Sets where the FPFwErrorRaise function will return to. 
 *
 *  @return
 *      True if FPFwErrorRaise has been called before.
 *      False if FPFwErrorRaise has not been called.
 */
bool set_error_handler_return();
