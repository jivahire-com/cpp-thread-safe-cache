//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file newlib_lock.h
 * Header for the newlib lock implementation
 */

#pragma once

/*----------- includes ------------*/
#include <stdint.h>
#include <tx_api.h>
#ifndef _WIN32
    #include <sys/lock.h>
#else
typedef void* _LOCK_T;
#endif
/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct {
    TX_MUTEX mutex;
    uint32_t owner;
    uint32_t recursion;
    volatile uint32_t in_use;
} threadx_lock_t;

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __retarget_lock_init(_LOCK_T* lock);
void __retarget_lock_close(_LOCK_T lock);
void __retarget_lock_acquire(_LOCK_T lock);
int __retarget_lock_try_acquire(_LOCK_T lock);
void __retarget_lock_release(_LOCK_T lock);
void __retarget_lock_init_recursive(_LOCK_T* lock);
void __retarget_lock_close_recursive(_LOCK_T lock);
void __retarget_lock_acquire_recursive(_LOCK_T lock);
int __retarget_lock_try_acquire_recursive(_LOCK_T lock);
void __retarget_lock_release_recursive(_LOCK_T lock);