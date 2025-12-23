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
#include <stdbool.h>
#include <tx_thread.h>

/*-- Symbolic Constant Macros (defines) --*/
#define LOCK_NAME_NORMAL      "nl_lock"
#define LOCK_NAME_RECURSIVE   "nl_rlock"
#define NEWLIB_LOCK_CAST(x)   ((threadx_lock_t*)(x))
#define NEWLIB_LOCK_POOL_SIZE 8
/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
// Newlib pre-defined global lock.
struct __lock __lock___arc4random_mutex;
struct __lock __lock___at_quick_exit_mutex;
struct __lock __lock___atexit_recursive_mutex;
struct __lock __lock___dd_hash_mutex;
struct __lock __lock___env_recursive_mutex;
struct __lock __lock___malloc_recursive_mutex;
struct __lock __lock___sfp_recursive_mutex;
struct __lock __lock___tz_mutex;

// Lock pool for init lock with allocation.
static struct __lock g_lock_pool[NEWLIB_LOCK_POOL_SIZE] = {0};
extern TX_THREAD _tx_timer_thread;

/*------------- Functions ----------------*/
static _LOCK_T lock_alloc(void)
{
    for (UINT i = 0; i < NEWLIB_LOCK_POOL_SIZE; ++i)
    {
        if (g_lock_pool[i].in_use == 0)
        {
            return &g_lock_pool[i];
        }
    }

    return NULL;
}

static void lock_init(_LOCK_T lock, bool recursive)
{
    if (lock)
    {
        if (lock->in_use == 0) // Make sure mutex is not created yet.
        {
            if (recursive)
            {
                (void)tx_mutex_create(&lock->mutex, LOCK_NAME_RECURSIVE, TX_INHERIT);
            }
            else
            {
                FpFwLockInitialize(&lock->lock);
                lock->lock_state = 0;
            }
            lock->in_use = 1;
        }
    }
}

int in_timer_context(void)
{
    return tx_thread_identify() == &_tx_timer_thread;
}

int in_isr_context(void)
{
    return (TX_THREAD_GET_SYSTEM_STATE() != 0);
}

void __retarget_lock_init(_LOCK_T* lock)
{
    // If lock needs to be allocated, alloc from pool.
    if (*lock == NULL)
    {
        *lock = lock_alloc();
    }

    lock_init(*lock, false);
}

void __retarget_lock_close(_LOCK_T lock)
{
    if (lock)
    {
        if (lock->lock_state != 0)
        {
            FpFwLockRelease(&lock->lock, lock->lock_state);
        }
        lock->lock.Signature = 0;
        lock->lock_state = 0;
        lock->in_use = 0;
    }
}

void __retarget_lock_acquire(_LOCK_T lock)
{
    if (lock)
    {
        if (lock->in_use == 0)
        {
            lock_init(lock, false);
        }

        lock->lock_state = FpFwLockAcquire(&lock->lock);
    }
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
    if (lock)
    {
        if (lock->in_use == 0)
        {
            lock_init(lock, false);
        }

        if (lock->lock_state == 0)
        {
            lock->lock_state = FpFwLockAcquire(&lock->lock);
            return 0;
        }
    }

    // Already locked or null lock.
    return -1;
}

void __retarget_lock_release(_LOCK_T lock)
{
    if (lock && lock->in_use == 1 && lock->lock_state != 0)
    {
        FpFwLockRelease(&lock->lock, lock->lock_state);
        lock->lock_state = 0;
    }
}

void __retarget_lock_init_recursive(_LOCK_T* lock)
{
    // If lock needs to be allocated, alloc from pool.
    if (*lock == NULL)
    {
        *lock = lock_alloc();
    }

    lock_init(*lock, true);
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
    if (lock)
    {
        (void)tx_mutex_delete(&lock->mutex);
        lock->in_use = 0;
    }
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    if (!lock || in_isr_context() || in_timer_context())
    {
        return;
    }

    if (lock->in_use == 0)
    {
        lock_init(lock, true);
    }

    (void)tx_mutex_get(&lock->mutex, TX_WAIT_FOREVER);
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
    if (!lock)
    {
        return -1;
    }

    if (in_isr_context() || in_timer_context())
    {
        return 0;
    }

    if (lock->in_use == 0)
    {
        lock_init(lock, true);
    }

    return tx_mutex_get(&lock->mutex, 0) == TX_SUCCESS ? 0 : -1;
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
    if (lock && lock->in_use == 1 && !in_isr_context() && !in_timer_context())
    {
        (void)tx_mutex_put(&lock->mutex);
    }
}
