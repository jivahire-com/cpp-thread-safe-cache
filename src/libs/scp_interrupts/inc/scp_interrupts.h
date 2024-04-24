//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file scp_interrupts.h
*/

#pragma once

/*--------------- Includes ---------------*/

/*------------- Typedefs -----------------*/
 typedef enum _IRQn_t {
    #include <kng_scp_ints.h>
} IRQn_t;
