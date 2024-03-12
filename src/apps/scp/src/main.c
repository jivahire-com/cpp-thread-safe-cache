//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file main.c
 * This file serves the main entry point for the scp app
 */

/*------------- Includes -----------------*/

#include <DfwkThreadXHost.h>
#include <debug.h>
#include <scp_event_trace_collector.h>
#include <scp_events.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>
#include <uart.h>

/*-- Symbolic Constant Macros (defines) --*/
#define KB                  (1024)
#define STACK_MEM_POOL_SIZE (32 * KB)
#define MAIN_STACK_SIZE     (4 * KB)
#define DFWK_STACK_SIZE     (4 * KB)

/*--------- Typedefs ----------*/

/*--------- Function Prototypes ----------*/

void main_thread(ULONG thread_input);

/*-- Declarations (Statics and globals) --*/
static uint8_t s_stack_pool_memory[STACK_MEM_POOL_SIZE];
static TX_BYTE_POOL s_stack_mem_pool_ctrl;

static TX_THREAD s_main_thread;
static uint8_t* s_main_stack;

static uint8_t* s_dfwk_stack;
static DFWK_THREADX_HOST s_dfwk_host;

/*-------------- Functions ---------------*/
static void soc_init(void)
{
    UartInit(UART0BASE_SCP);
    DebugInit();
}

// Prior to main, an assembly function initializes .bss and invokes constructors
// see cortexm7_vectors.S for the initial reset_handler entrypoint
int main(void)
{

    soc_init();

    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();

    /* Keep compiler happy, we should never return from the kernel */
    return 0;
}

/**
 *      The tx_application_define() function executes after the basic ThreadX initialization is complete.
 *      It is responsible for setting up all of the initial system resources, including threads, queues,
 *      semaphores, mutexes, event flags, and memory pools.
 *
 *  @param firstUnusedMemory
 *      Points to the first-available RAM address.  It is typically used as a starting point for initial
 *      run-time memory allocations of thread stacks, queues, and memory pools.
 */
void tx_application_define(void* firstUnusedMemory)
{
    // firstUnusedMemory is derived from __RAM_segment_used_end__ + 4 bytes
    // in SCP we have no use for this pointer
    UNUSED(firstUnusedMemory);

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    (void)tx_byte_pool_create(&s_stack_mem_pool_ctrl, "stack byte pool", &s_stack_pool_memory, STACK_MEM_POOL_SIZE);

    (void)tx_byte_allocate(&s_stack_mem_pool_ctrl, (VOID**)&s_main_stack, MAIN_STACK_SIZE, TX_NO_WAIT);

    // We need to do some initialization on a thread in order to allow blocking during the initialization routine
    (void)tx_thread_create(&s_main_thread,
                           "scp_main",
                           main_thread,
                           0, // Entry input
                           s_main_stack,
                           MAIN_STACK_SIZE,
                           1, // Priority
                           1, // Preempt Threshold
                           TX_NO_TIME_SLICE,
                           TX_AUTO_START);

    (void)tx_byte_allocate(&s_stack_mem_pool_ctrl, (VOID**)&s_dfwk_stack, DFWK_STACK_SIZE, TX_NO_WAIT);

    scp_etc_initialize();
}

// The main initialization routine executes on a thread to allow the use of synchronization objects
void main_thread(ULONG thread_input)
{
    uint32_t count = 0;
    UNUSED(thread_input);

    printf("\nHello World - SCP!\n");

    (void)DfwkThreadxHostInitialize(&s_dfwk_host, s_dfwk_stack, DFWK_STACK_SIZE, 1, 1, TX_NO_TIME_SLICE);

    // Do nothing
    while (true)
    {
        tx_thread_sleep(2);
        FPFW_ET_LOG(ScpHeartBeat, count++);
    }
}
