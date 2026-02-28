//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump.c
 *  Crash dump public API implementation.
 */

/*--------------- Includes ---------------*/
#include "crash_dump_gpio.h" // for cd_gpio_assert_cd_in_progress
#include "crash_dump_icc.h"
#include "crash_dump_pldm.h"
#include "crash_dump_status.h" // for crash_dump_update_core_state

#include <FpFwUtils.h>         // for FPFW_UNUSED
#include <cmsis_m7.h>          // for __WFI
#include <crash_dump.h>        // for crash_dump_handler
#include <crash_dump_events.h> // for CRASH_DUMP_ET
#include <crash_dump_memory.h> // for crash dump memory layout
#include <fpfw_cfg_mgr.h>      // for knobs
#include <idsw_kng.h>          // for IS_PLATFORM_SVP
#include <nvic.h>              // for nvic_get_current_irq
#include <stdint.h>            // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/
#define CRASH_DUMP_MAX_WAIT_FOR_MCP_MINI_COUNT 20
#define CRASH_DUMP_WAIT_FOR_MCP_MINI_DELAY_US  100000 // 100 ms

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
core_crash_context_t g_core_crash_context;
static volatile bool s_bug_check_initiated_crash = false;
static volatile bool s_is_ue = false;
static bool g_utc_ready = false;
static uint32_t cd_origin_core_id = 0;

/*------------- Functions ----------------*/
void crash_dump_svp_probe(uint32_t die_index, uint32_t core_index, bool is_full, uint64_t dump_address, uint64_t dump_size)
{
    // Do Nothing but wait SVP crashdump probe generate dump file
    FPFwCDPrintf("%s: Die%ld Core%ld: %s dump address 0x%08lx%08lx size 0x%08lx%08lx\n",
                 __FUNCTION__,
                 die_index,
                 core_index,
                 is_full ? "Full" : "Mini",
                 (uint32_t)(dump_address >> 32),
                 (uint32_t)(dump_address & 0xFFFFFFFF),
                 (uint32_t)(dump_size >> 32),
                 (uint32_t)(dump_size & 0xFFFFFFFF));
}

void crash_dump_set_UE(bool is_ue)
{
    s_is_ue = is_ue;
}

bool crash_dump_is_UE(void)
{
    return s_is_ue;
}

bool crash_dump_bug_check_initiated_dump()
{
    return s_bug_check_initiated_crash;
}

void crash_dump_utc_ready(bool ready)
{
    g_utc_ready = ready;
}

bool crash_dump_is_utc_ready(void)
{
    return g_utc_ready;
}

FPFW_NORETURN void crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    __disable_irq();

    s_bug_check_initiated_crash = true;

    uint32_t nvic_irq_num = 0;
    nvic_status_t nvic_status = nvic_get_current_irq(&nvic_irq_num);

    if (nvic_status == NVIC_STATUS_SUCCESS)
    {
        // Lower the interrupt priority to 1 if it is called by the other ISR.
        // This is to ensure DebugMonitor_IRQn interrupt, otherwise HardFault will occur.
        NVIC_SetPriority(nvic_irq_num, 1);
    }

    if (errorCode == 0)
    {
        // Specify a non-zero error code for bug check initiated dump with success code.
        errorCode = KNG_BGCHK_ZERO_ERROR;
    }
#ifndef _WIN32
    // Ensure args are put into specific registers, so they can later be recovered by the debug handler
    __asm__ volatile("mov r0, %[code]\n"
                     "mov r1, %[p1]\n"
                     "mov r2, %[p2]\n"
                     "mov r3, %[p3]\n"
                     "mov r4, %[p4]\n"
                     : // No output
                     : // Inputs
                     [code] "r"(errorCode),
                     [p1] "r"(p1),
                     [p2] "r"(p2),
                     [p3] "r"(p3),
                     [p4] "r"(p4)
                     : // Clobbers
                     "r0", "r1", "r2", "r3", "r4");
#else
    FPFW_UNUSED(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);
#endif
    // Initiate a Debug Monitor exception, which will call the FPFwCDCrashDumpHandler
    __enable_irq();
#ifndef _WIN32
    __asm__ volatile("bkpt \n");
#endif

    // The exception handler will hang the core, but this is needed to respect the noreturn attribute
    crash_dump_wait_forever();
}

FPFW_NORETURN void crash_dump_bug_check_external(uint32_t origin_id)
{
    cd_origin_core_id = origin_id;
    CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_EXTERNAL_BUGCHECK, cd_origin_core_id);

    uint32_t nvic_irq_num = 0;
    nvic_status_t nvic_status = nvic_get_current_irq(&nvic_irq_num);

    if (nvic_status == NVIC_STATUS_SUCCESS)
    {
        // Lower the interrupt priority to 1 if it is called by the other ISR.
        // This is to ensure DebugMonitor_IRQn interrupt, otherwise HardFault will occur.
        NVIC_SetPriority(nvic_irq_num, 1);
    }
#ifndef _WIN32
    __asm__ volatile("bkpt\n");
#endif

    // The exception handler will hang the core, but this is needed to respect the noreturn attribute
    crash_dump_wait_forever();
}

/**
 * Hangs the core by WFI indefinitely
 */
__attribute__((__weak__)) FPFW_NORETURN void crash_dump_wait_forever()
{
    FPFwCDPrintf("crash_dump_wait_forever\n");

    while (true)
    {
        __WFI();
    }
}

static void finalize_SCP_mini_dump(crash_dump_context_t* ctx, uint32_t errorCode)
{
    if (errorCode == 0 && (cd_origin_core_id & 0xFFFF) == CRASH_DUMP_CORE_MCP)
    {
        uint32_t retry_count = 0;

        // CTI triggerred SCP crash.
        // Wait until MCP mini dump is completed and switch to MCP mini dump if valid.
        crash_dump_core_state_t mcp_state =
            crash_dump_core_state(CRASH_DUMP_TYPE_MINI, ctx->die_index, CRASH_DUMP_CORE_MCP);

        while (mcp_state == CRASH_DUMP_STATE_IN_PROGRESS && retry_count < CRASH_DUMP_MAX_WAIT_FOR_MCP_MINI_COUNT)
        {
            // Wait for MCP mini dump to complete
            SLEEP_US(CRASH_DUMP_WAIT_FOR_MCP_MINI_DELAY_US); // Sleep for 100ms before checking again
            mcp_state = crash_dump_core_state(CRASH_DUMP_TYPE_MINI, ctx->die_index, CRASH_DUMP_CORE_MCP);
            retry_count++;
        }

        if (mcp_state == CRASH_DUMP_STATE_COMPLETED)
        {
            FPFwCDPrintf("MCP mini dump completed, finalize SCP mini dump with MCP mini dump.\n");
            memcpy((void*)CRASH_DUMP_MINI_SCP_ADDR, (void*)CRASH_DUMP_MINI_MCP_ADDR, CRASH_DUMP_MINI_SCP_SIZE);
        }
        else
        {
            FPFwCDPrintf("MCP mini dump is not completed or not available, finalize SCP mini dump.\n");
        }
    }

    crash_dump_update_core_state(ctx->type_ctx[CRASH_DUMP_TYPE_MINI], CRASH_DUMP_STATE_COMPLETED);
}

void crash_dump_handler(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    bool should_generate_crash_dump = config_get_crash_dump_enable();

    // If configured not to do crash dump process, simply return and hang in exception handler
    if (should_generate_crash_dump)
    {
        crash_dump_context_t* ctx = crash_dump_context();
        bool finalize_scp_mini_dump = false;

        for (int i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
        {
            if (ctx->type_ctx[i] != NULL)
            {
                // Update the crash dump state
                crash_dump_update_core_state(ctx->type_ctx[i], CRASH_DUMP_STATE_IN_PROGRESS);
            }
        }

        // Assert GPIO_CD_IN_PROGRESS
        cd_gpio_assert_cd_in_progress(true);

        cd_origin_core_id = errorCode == 0 ? cd_origin_core_id : CRASH_DUMP_PROCESSOR_ID(ctx->die_index, ctx->core_index);

        // Trigger remote entities to indicate this core has crashed if needed
        crash_dump_remote_trigger(s_is_ue, cd_origin_core_id);

        // Generate dumps
        FPFwCdBugCheckInfo bug_check_info = {};
        bug_check_info.coreIndex = CRASH_DUMP_PROCESSOR_ID(ctx->die_index, ctx->core_index);
        bug_check_info.data.Code = errorCode;
        bug_check_info.data.Parameter[0] = p1;
        bug_check_info.data.Parameter[1] = p2;
        bug_check_info.data.Parameter[2] = p3;
        bug_check_info.data.Parameter[3] = p4;

        for (int i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
        {
            if (ctx->type_ctx[i] != NULL)
            {
                // Dump the crash dump
                FPFwCDCrashDumpHandler(&ctx->type_ctx[i]->crash_dump_ctx, &g_core_crash_context, &bug_check_info);

                // Update the full crash dump state
                if (ctx->type_ctx[i]->type == CRASH_DUMP_TYPE_MINI && ctx->core_index == CRASH_DUMP_CORE_SCP && errorCode == 0)
                {
                    // For SCP CTI initiated mini dump, the state will be updated in finalize_SCP_mini_dump.
                    finalize_scp_mini_dump = true;
                }
                else
                {
                    crash_dump_update_core_state(ctx->type_ctx[i], CRASH_DUMP_STATE_COMPLETED);
                }

                if (IS_PLATFORM_SVP())
                {
                    crash_dump_svp_probe(ctx->die_index,
                                         ctx->core_index,
                                         ctx->type_ctx[i]->type == CRASH_DUMP_TYPE_FULL,
                                         ctx->type_ctx[i]->mem_ctx.baseAddr,
                                         ctx->type_ctx[i]->mem_ctx.nextAddr - ctx->type_ctx[i]->mem_ctx.baseAddr);
                }

                CRASH_DUMP_ET_INFO(CRASH_DUMP_ET_TYPE_CD_CRASH);
            }
        }

        if (finalize_scp_mini_dump)
        {
            // Finalize SCP mini dump after all dumps are generated.
            finalize_SCP_mini_dump(ctx, errorCode);
        }
    }
}

void crash_dump_register_mmio_register(volatile void* mmio_reg, uint32_t reg_count, FPFwCdDumpPriority priority)
{
    crash_dump_context_t* ctx = crash_dump_context();

    for (int i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
    {
        if (ctx->type_ctx[i] != NULL)
        {
            CdRegisterMMIORegisterSet(&ctx->type_ctx[i]->crash_dump_ctx, (uintptr_t)mmio_reg, reg_count, priority);
        }
    }
}

void crash_dump_register_address32(void* address, uint32_t size, FPFwCdDumpPriority priority)
{
    crash_dump_context_t* ctx = crash_dump_context();

    for (int i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
    {
        if (ctx->type_ctx[i] != NULL)
        {
            CdRegisterAddress32(&ctx->type_ctx[i]->crash_dump_ctx, address, size, priority);
        }
    }
}

void crash_dump_register_address64(uint64_t address, uint32_t size, FPFwCdDumpPriority priority)
{
    crash_dump_context_t* ctx = crash_dump_context();

    for (int i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
    {
        if (ctx->type_ctx[i] != NULL)
        {
            CdRegisterAddress64(&ctx->type_ctx[i]->crash_dump_ctx, address, size, priority);
        }
    }
}

void crash_dump_register_address32_pointer_array(FPFwCdDumpPriority priority,
                                                 uint32_t minChunkSize,
                                                 uint32_t maxRegistrationCount,
                                                 void** pointerArray,
                                                 uint32_t pointerArrayCount)
{
    crash_dump_context_t* ctx = crash_dump_context();

    for (int i = 0; i < CRASH_DUMP_TYPE_NUM; i++)
    {
        if (ctx->type_ctx[i] != NULL)
        {
            CdRegisterAddress32PointerArray(&ctx->type_ctx[i]->crash_dump_ctx,
                                            priority,
                                            minChunkSize,
                                            maxRegistrationCount,
                                            pointerArray,
                                            pointerArrayCount);
        }
    }
}