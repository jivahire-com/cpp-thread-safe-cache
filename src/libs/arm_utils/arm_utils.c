//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file arm_utils.c
 *
 * This file contains some common macros, variables and inline
 * functions used across the project that is specific to arm compiler for
 * MSCP and accelerators firmware.
 *
 */

/*------------- Includes -----------------*/
#include <arm_utils.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
int memcpy_s(void* dest, size_t dest_size, const void* src, size_t count)
{
    if (dest == NULL || src == NULL)
    {
        return -1;
    }

    if (count > dest_size)
    {
        return -1;
    }

    memcpy(dest, src, count);

    return 0;
}

int sprintf_s(char* dest, size_t dest_size, const char* format, ...)
{
    if (dest == NULL || format == NULL)
    {
        return -1;
    }
    va_list args;
    va_start(args, format);
    int result = vsnprintf(dest, dest_size, format, args);
    va_end(args);

    if (result < 0 || (size_t)result >= dest_size)
    {
        return -1;
    }
    return result;
}