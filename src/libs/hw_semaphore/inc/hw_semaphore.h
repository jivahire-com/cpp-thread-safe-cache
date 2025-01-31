//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hw_semaphore.h
 * Header file of HW semaphore
 */
#pragma once

/*----------- Nested includes ------------*/
#include <kng_error.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize HW semaphore
 * 
 * @return KNG_STATUS KNG_SUCCESS if successful, otherwise matched error code.
 */
KNG_STATUS init_hw_semaphore();