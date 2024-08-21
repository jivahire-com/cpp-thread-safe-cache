//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file bitops.h
 * This file contains macros and defines for bit related operations
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include "utils.h"
#include "arm_intrinsic.h"

/*------------------- Symbolic Constant Macros (defines) --------------------*/
#define SET_BIT_MASK(pos)               (0x1UL << pos)

#define SET_BIT(val, pos)               (val | (0x1UL << pos))
#define CLEAR_BIT(val, pos)             ((val | (0x1UL << pos)) ^ (0x1UL << pos))

#define BITWISE_INVERT(val)             (~val)
#define BITWISE_AND(val1, val2)         (val1 & val2)
#define BITWISE_OR(val1, val2)          (val1 | val2)

// Reverse the bits and then get count of leading zeros
// 0x0000_0002 -- RBIT --> 0x4000_0000 -- CLZ --> 1
// TODO: Task 1938438: [SDM] Move ARM specific function calls like __DMB(), __RBIT() to CortexM7 Repo
#define GET_LOWEST_SET_BIT_INDEX(val)   ((__CLZ(__RBIT(val))))

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/
