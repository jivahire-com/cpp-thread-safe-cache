//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file warm_start_test_mocks.h
 * Mocks for warm_start
 */

/*------------- Includes -----------------*/

#include <CMockaWrapper.h>
#include <cstddef>
#include <cstdint>
extern "C" {

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*----------- Mock Functions -------------*/
UINT __wrap__txe_mutex_create(TX_MUTEX* mutex_ptr, CHAR* name_ptr, UINT inherit, UINT mutex_control_block_size);

UINT __wrap__txe_mutex_get(TX_MUTEX* mutex_ptr, ULONG wait_option);

UINT __wrap__txe_mutex_put(TX_MUTEX* mutex_ptr);
}