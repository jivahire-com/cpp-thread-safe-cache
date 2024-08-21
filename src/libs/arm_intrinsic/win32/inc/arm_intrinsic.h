//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file arm_intrinsic.h
 *  For windows build.
*/

#pragma once

/*--------------- Includes ---------------*/
#include <stdint.h>

/*------------- Typedefs -----------------*/

/**
 *
 *    Data Synchronization Barrier 
 *
 *    @brief Acts as a special kind of Data Memory Barrier.
 *           It completes when all explicit memory accesses before this instruction complete.
 *
 */
#define __DSB() 

/*--------- Function Prototypes ----------*/

/**
 * __RBIT function is specific to ARM ISA
 */
uint32_t __RBIT(uint32_t val);


