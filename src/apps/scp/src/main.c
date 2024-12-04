//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file main.c
 * This file serves the main entry point for the scp app
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <fpfw_init.h>
#include <scmi_prim.h>
#include <scp_events.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define KB                  (1024)
#define STACK_MEM_POOL_SIZE (32 * KB)
#define MAIN_STACK_SIZE     (4 * KB)
#define DFWK_STACK_SIZE     (4 * KB)
#define SLEEP_TICKS         (2)
/*--------- Typedefs ----------*/

/*--------- Function Prototypes ----------*/

void main_thread(ULONG thread_input);

/*-- Declarations (Statics and globals) --*/
static uint8_t s_stack_pool_memory[STACK_MEM_POOL_SIZE];
static TX_BYTE_POOL s_stack_mem_pool_ctrl;

static TX_THREAD s_main_thread;
static uint8_t* s_main_stack;

extern fpfw_init_component_t _data_fpfw_init_start;
extern fpfw_init_component_t _data_fpfw_init_end;

/*-------------- Functions ---------------*/

// Prior to main, an assembly function initializes .bss and invokes constructors
// see cortexm7_vectors.S for the initial reset_handler entrypoint
int main(void)
{
    // uncomment to debug early init with headless SVP
    // volatile bool go = false;
    // while (!go)
    // {
    // }

    /* Enter the ThreadX kernel. Performing Low and High level initialization. */
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
    FPFW_UNUSED(firstUnusedMemory);

    /* ThreadX Component Utilization */

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

    // Initialize components defined by FPFW init library
    fpfw_init(&_data_fpfw_init_start, &_data_fpfw_init_end);
}

// The main initialization routine executes on a thread to allow the use of synchronization objects
void main_thread(ULONG thread_input)
{
    FPFW_UNUSED(thread_input);

    printf("\nHello World - SCP!\n");

    scmi_init();
    // Do nothing
    uint32_t rtos_ticks = 0;
    while (true)
    {
        if (rtos_ticks % 100 == 0)
        {
            // reduce print frequency
            FPFW_ET_LOG(ScpHeartBeat, rtos_ticks);
        }
        tx_thread_sleep(SLEEP_TICKS);
        rtos_ticks += SLEEP_TICKS;
        // **TODO will have to remove this once the SCMI Driver framework gets implemented
        // https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1903038
        (void)scmi_poll_message();
    }
}
