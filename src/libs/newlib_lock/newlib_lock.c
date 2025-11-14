//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file newlib_lock.c
 * This file contains the implementation of newlib locks for reentrancy.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <newlib_lock.h>
#include <tx_thread.h>

/*-- Symbolic Constant Macros (defines) --*/
#define LOCK_NAME_NORMAL      "nl_lock"
#define LOCK_NAME_RECURSIVE   "nl_rlock"
#define NEWLIB_LOCK_CAST(x)   ((threadx_lock_t*)(x))
#define NEWLIB_LOCK_POOL_SIZE 8
/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
int __lock___arc4random_mutex;
int __lock___at_quick_exit_mutex;
int __lock___atexit_recursive_mutex;
int __lock___dd_hash_mutex;
int __lock___env_recursive_mutex;
int __lock___malloc_recursive_mutex;
int __lock___sfp_recursive_mutex;
int __lock___tz_mutex;

static threadx_lock_t g_lock_pool[NEWLIB_LOCK_POOL_SIZE];

/*------------- Functions ----------------*/
static threadx_lock_t* lock_alloc(void)
{
    for (UINT i = 0; i < NEWLIB_LOCK_POOL_SIZE; ++i)
    {
        if (g_lock_pool[i].in_use == 0)
        {
            g_lock_pool[i].in_use = 1;
            return &g_lock_pool[i];
        }
    }

    return (threadx_lock_t*)0;
}

static inline threadx_lock_t* as_lock(_LOCK_T h)
{
    return (threadx_lock_t*)h;
}

void __retarget_lock_init(_LOCK_T* lock)
{
    threadx_lock_t* threadx_lock = lock_alloc();

    if (!threadx_lock)
    {
        *lock = (void*)0;
        return;
    }

    (void)tx_mutex_create(&threadx_lock->mutex, LOCK_NAME_NORMAL, TX_NO_INHERIT);
    threadx_lock->owner = 0;
    threadx_lock->recursion = 0;
    *lock = (void*)threadx_lock;
}

void __retarget_lock_close(_LOCK_T lock)
{
    threadx_lock_t* threadx_lock = as_lock(lock);
    if (threadx_lock)
    {
        (void)tx_mutex_delete(&threadx_lock->mutex);
        threadx_lock->owner = 0;
        threadx_lock->recursion = 0;
        threadx_lock->in_use = 0;
    }
}

void __retarget_lock_acquire(_LOCK_T lock)
{
    threadx_lock_t* threadx_lock = as_lock(lock);

    if (!threadx_lock || (TX_THREAD_GET_SYSTEM_STATE() != 0))
    {
        return;
    }

    (void)tx_mutex_get(&threadx_lock->mutex, TX_WAIT_FOREVER);
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
    threadx_lock_t* threadx_lock = (threadx_lock_t*)lock;

    if (!threadx_lock)
    {
        return -1;
    }

    if (TX_THREAD_GET_SYSTEM_STATE() != 0)
    {
        return 0;
    }

    return (tx_mutex_get(&threadx_lock->mutex, 0) == TX_SUCCESS) ? 0 : -1;
}

void __retarget_lock_release(_LOCK_T lock)
{
    threadx_lock_t* threadx_lock = as_lock(lock);
    if (!threadx_lock || (TX_THREAD_GET_SYSTEM_STATE() != 0))
    {
        return;
    }

    (void)tx_mutex_put(&threadx_lock->mutex);
}

void __retarget_lock_init_recursive(_LOCK_T* lock)
{
    threadx_lock_t* threadx_lock = lock_alloc();

    if (!threadx_lock)
    {
        *lock = (void*)0;
        return;
    }

    (void)tx_mutex_create(&threadx_lock->mutex, LOCK_NAME_RECURSIVE, TX_INHERIT);
    threadx_lock->owner = 0;
    threadx_lock->recursion = 0;
    *lock = (void*)threadx_lock;
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
    threadx_lock_t* threadx_lock = as_lock(lock);

    if (threadx_lock)
    {
        (void)tx_mutex_delete(&threadx_lock->mutex);
        threadx_lock->owner = 0;
        threadx_lock->recursion = 0;
        threadx_lock->in_use = 0;
    }
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    threadx_lock_t* threadx_lock = as_lock(lock);

    if (!threadx_lock || (TX_THREAD_GET_SYSTEM_STATE() != 0))
    {
        return;
    }

    uint32_t me = (uint32_t)tx_thread_identify();

    if (threadx_lock->owner == me)
    {
        threadx_lock->recursion++;
        return;
    }

    (void)tx_mutex_get(&threadx_lock->mutex, TX_WAIT_FOREVER);
    threadx_lock->owner = me;
    threadx_lock->recursion = 1;
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    threadx_lock_t* threadx_lock = (threadx_lock_t*)lock;
    if (!threadx_lock)
    {
        return -1;
    }

    if (TX_THREAD_GET_SYSTEM_STATE() != 0)
    {
        return 0;
    }

    uint32_t me = (uint32_t)tx_thread_identify();

    if (threadx_lock->owner == me)
    {
        threadx_lock->recursion++;
        return 0;
    }

    if (tx_mutex_get(&threadx_lock->mutex, 0) != TX_SUCCESS)
    {
        return -1;
    }

    threadx_lock->owner = me;
    threadx_lock->recursion = 1;
    return 0;
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
    threadx_lock_t* threadx_lock = as_lock(lock);

    if (!threadx_lock || (TX_THREAD_GET_SYSTEM_STATE() != 0))
    {
        return;
    }

    uint32_t me = (uint32_t)tx_thread_identify();

    if (threadx_lock->owner != me)
    {
        return;
    }

    if (threadx_lock->recursion > 1)
    {
        threadx_lock->recursion--;
        return;
    }

    threadx_lock->recursion = 0;
    threadx_lock->owner = 0;
    (void)tx_mutex_put(&threadx_lock->mutex);
}
