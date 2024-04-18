//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file arm_utils.h
 * This file contains some common macros, variables and inline 
 * functions used across the project that is specific to arm compiler for
 * MSCP and accelerators firmware.
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>
#include <stdlib.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief 
 * 
 * @param dest 
 * @param dest_size 
 * @param src 
 * @param count 
 * @return int 
 */
int memcpy_s(void *dest, size_t dest_size, const void *src, size_t count);

/**
 * @brief 
 * 
 * @param dest 
 * @param dest_size 
 * @param format 
 * @param ... 
 * @return int 
 */
int sprintf_s(char *dest, size_t dest_size, const char *format, ...);