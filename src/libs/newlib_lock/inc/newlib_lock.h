//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file newlib_lock.h
 * Header for the newlib lock implementation
 */

#pragma once

/*----------- includes ------------*/
#include <FpFwLock.h>
#include <stdint.h>
#include <tx_api.h>
#ifndef _WIN32
    #include <sys/lock.h>
#else
struct __lock;
typedef struct __lock * _LOCK_T;
#define _LOCK_RECURSIVE_T _LOCK_T
#endif
/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
struct __lock
{
    // Use FPFW_LOCK for non-recursive lock.
    FPFW_LOCK lock;
    FPFW_LOCK_STATE lock_state;
    // Use TX_MUTEX for recursive lock.
    TX_MUTEX mutex;
    volatile uint32_t in_use;
};

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