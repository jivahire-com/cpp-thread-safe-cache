//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_payload_threadx.c
 * Registering threadX data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include "crash_dump_payload.h" // for CD_THREADX_DATA, CD_GUID

#include <FpFwUtils.h>                // for FPFW_UNUSED
#include <crash_dump.h>               // for crash_dump_register_address32
#include <modules/CdDumpDescriptor.h> // for _FPFwCdDumpPriority
#include <modules/CdDumpManager.h>    // for CdRegisterCallback
#include <string.h>                   // for strlen, NULL
#include <tx_thread.h>                // for _tx_thread_created_count, _tx_...

/*-- Symbolic Constant Macros (defines) --*/
/**
 * GUID used to locate the THREADX data in Crash dump
 * GUID : 93f3d4c2-5729-4fa3-be94-6ab263a76730
 */
#define CD_THREADX_GUID                                    \
    {                                                      \
        0x93f3d4c2, 0x5729, 0x4fa3,                        \
        {                                                  \
            0xbe, 0x94, 0x6a, 0xb2, 0x63, 0xa7, 0x67, 0x30 \
        }                                                  \
    }
#define CD_DUMP_STACK_ADDRESS_CHUNKS     128
#define CD_DUMP_STACK_ADDRESS_CHUNK_SIZE 128

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
static CD_THREADX_DATA cdThreadXData;

/*--------- Function Prototypes ----------*/

/*------------- Functions ----------------*/
/**
 * @brief Register threadX data capture callback.
 *
 */
void crash_dump_register_threadx(crash_dump_type_context_t* type_context)
{
    CdRegisterCallback(&type_context->crash_dump_ctx, crash_dump_capture_threadx, type_context, FPFW_CD_DUMP_PRIORITY_CRITICAL);
}

/**
 * @brief Captures threadX data
 *
 * @param context
 */
void crash_dump_capture_threadx(void* context)
{
    crash_dump_type_context_t* type_context = (crash_dump_type_context_t*)context;
    FPFwCrashDumpCtx* ctx = &type_context->crash_dump_ctx;
    /**
     * The following variables are defined in tx_thread_initialize.c.  We need
     * the pointers and the data they point to.
     *      void*       _tx_thread_system_stack_ptr
     *      TX_THREAD*  _tx_thread_current_ptr
     *      TX_THREAD*  _tx_thread_execute_ptr
     *      TX_THREAD*  _tx_thread_created_ptr
     *      ULONG       _tx_thread_created_count
     *      ULONG       _tx_thread_system_state
     */

    /* Storing the ThreadX data with a GUID in crash dump so that they can
    easily found while parsing later */

    cdThreadXData.guid = (CD_GUID)CD_THREADX_GUID;
    cdThreadXData._tx_thread_system_stack_ptr = _tx_thread_system_stack_ptr;
    cdThreadXData._tx_thread_current_ptr = _tx_thread_current_ptr;
    cdThreadXData._tx_thread_execute_ptr = _tx_thread_execute_ptr;
    cdThreadXData._tx_thread_created_ptr = _tx_thread_created_ptr;
    cdThreadXData._tx_thread_created_count = _tx_thread_created_count;
    cdThreadXData._tx_thread_system_state = _tx_thread_system_state;

    CdRegisterAddress32(ctx, &cdThreadXData, sizeof(cdThreadXData), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    CdRegisterAddress32(ctx, &_tx_thread_system_stack_ptr, sizeof(void*), FPFW_CD_DUMP_PRIORITY_CRITICAL);
    CdRegisterAddress32(ctx, &_tx_thread_current_ptr, sizeof(void*), FPFW_CD_DUMP_PRIORITY_CRITICAL);
    CdRegisterAddress32(ctx, &_tx_thread_execute_ptr, sizeof(void*), FPFW_CD_DUMP_PRIORITY_CRITICAL);
    CdRegisterAddress32(ctx, &_tx_thread_created_ptr, sizeof(void*), FPFW_CD_DUMP_PRIORITY_CRITICAL);
    CdRegisterAddress32(ctx, &_tx_thread_created_count, sizeof(_tx_thread_created_count), FPFW_CD_DUMP_PRIORITY_CRITICAL);
    CdRegisterAddress32(ctx, (void*)&_tx_thread_system_state, sizeof(_tx_thread_system_state), FPFW_CD_DUMP_PRIORITY_CRITICAL);

    uint32_t threadCount = 0;

    for (TX_THREAD* pThread = _tx_thread_created_ptr; pThread != NULL && threadCount < _tx_thread_created_count;
         pThread = pThread->tx_thread_created_next, threadCount++)
    {
        // Register the thread control block
        CdRegisterAddress32(ctx, pThread, sizeof(*pThread), FPFW_CD_DUMP_PRIORITY_CRITICAL);

        // Register the stack data
        CdRegisterAddress32(ctx,
                            pThread->tx_thread_stack_start,
                            pThread->tx_thread_stack_end - pThread->tx_thread_stack_start,
                            FPFW_CD_DUMP_PRIORITY_CRITICAL);
        CdRegisterAddress32(ctx, pThread->tx_thread_name, strlen(pThread->tx_thread_name) + 1, FPFW_CD_DUMP_PRIORITY_CRITICAL);

        CdRegisterAddress32PointerArray(ctx,
                                        FPFW_CD_DUMP_PRIORITY_OPPORTUNISTIC,
                                        CD_DUMP_STACK_ADDRESS_CHUNK_SIZE,
                                        CD_DUMP_STACK_ADDRESS_CHUNKS,
                                        pThread->tx_thread_stack_start,
                                        (pThread->tx_thread_stack_end - pThread->tx_thread_stack_start) / sizeof(void*));
    }
}
