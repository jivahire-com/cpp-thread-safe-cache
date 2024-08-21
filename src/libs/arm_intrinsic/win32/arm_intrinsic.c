//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file arm_intrinsic.c
 *  For windows build.
 */

/*--------------- Includes ---------------*/
#include <arm_intrinsic.h>

/*------------- Global Functions -----------------*/

uint32_t __RBIT(uint32_t val)
{
    uint32_t out_val = 0x0;

    for (int i = 0; i < 32; i++)
    {
        out_val = out_val << 0x1;
        out_val = out_val | (val & 0x1);
        val = val >> 1;
    }

    return out_val;
}
