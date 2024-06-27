//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file interrupts.h
*/

#pragma once

/*--------------- Includes ---------------*/

/*------------- Typedefs -----------------*/
 typedef enum _SCP_IRQn_t {
    #include <kng_scp_ints.h>
} SCP_IRQn_t;
