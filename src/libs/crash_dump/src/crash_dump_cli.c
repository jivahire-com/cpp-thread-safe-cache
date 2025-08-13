//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash dump cli commands
 */

/*------------- Includes -----------------*/
#include "crash_dump_icc.h"
#include "crash_dump_status.h"

#include <FpFwCli.h>           // for FPFW_CLI_COMMAND, FpFwCliRegisterTable
#include <FpFwUtils.h>         // for FPFW_ARRAY_SIZE, FPFW_UNUSED
#include <bug_check.h>         // for BUG_CHECK
#include <crash_dump.h>        // for crash_dump_register_address32
#include <crash_dump_events.h> // for CRASH_DUMP_ET
#include <kng_error.h>         // for KNG_SUCCESS
#include <string.h>            // for strlen
#include <tx_api.h>            // for tx_thread_sleep
#include <tx_thread.h>         // for _tx_thread_current_ptr
#include <utils.h>             // for PLACED_CODE

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
static FPFW_CLI_STATUS cd_register_beef(int argc, const char** pp_argv);
static FPFW_CLI_STATUS cd_register_string(int argc, const char** pp_argv);
static FPFW_CLI_STATUS cd_bug_check(int argc, const char** pp_argv);
static FPFW_CLI_STATUS cd_trigger_exception(int argc, const char** pp_argv);
static FPFW_CLI_STATUS cd_set_single_core_mode(int argc, const char** pp_argv);
static FPFW_CLI_STATUS cd_stack_overflow(int argc, const char** pp_argv);
static FPFW_CLI_STATUS cd_get_status(int argc, const char** pp_argv);
static FPFW_CLI_STATUS cd_set_status(int argc, const char** pp_argv);
static FPFW_CLI_STATUS cd_set_core_status(int argc, const char** pp_argv);
static FPFW_CLI_STATUS cd_transfer_dump(int argc, const char** pp_argv);

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND s_cd_cmd_list[] = {
    {NULL_LIST_ENTRY, "crashdump", "reg_beef", cd_register_beef, "Registers filler hex as data to be stored in Crash Dump", "Usage: reg_beef (no arguments)"},
    {NULL_LIST_ENTRY, "crashdump", "reg_string", cd_register_string, "Registers a string to be stored in a crash dump", "Usage: reg_string <string>"},
    {NULL_LIST_ENTRY, "crashdump", "bc", cd_bug_check, "Invokes a bugcheck with the given error code", "Usage: bc <error code>"},
    {NULL_LIST_ENTRY, "crashdump", "single", cd_set_single_core_mode, "Generates single core crash dump", "Usage: single <0: multi-core, 1: single-core>"},
    {NULL_LIST_ENTRY, "crashdump", "trig_except", cd_trigger_exception, "Triggers an exception, causing a fault handler to execute", "Usage: trig_except (no arguments)"},
    {NULL_LIST_ENTRY, "crashdump", "st_over", cd_stack_overflow, "Causes stack overflow to test crash dump behavior", "Usage: st_over <suspend time tick>"},
    {NULL_LIST_ENTRY, "crashdump", "get_status", cd_get_status, "Get crash dump header status", "Usage: get_status"},
    {NULL_LIST_ENTRY, "crashdump", "set_status", cd_set_status, "Update crash dump header status", "Usage: set_status <type: 0: mini, 1: full> <0: Not in use, 1: In use, 2: in transfer>"},
    {NULL_LIST_ENTRY, "crashdump", "set_core_status", cd_set_core_status, "Update crash dump core status", "Usage: set_core_status <type: 0: mini, 1: full> <core index> <core status: 0: NA, 1:RD, 2:IP, 3:CP>"},
    {NULL_LIST_ENTRY, "crashdump", "transfer", cd_transfer_dump, "Transfer dump to BMC", "Usage: transfer"},
};

static uint32_t dead_beef = 0xDEADBEEF;
static uint32_t beef_cafe = 0xBEEFCAFE;

extern uint8_t __stack_start__;
extern uint8_t __stack_end__;

/*------------- Functions ----------------*/
PLACED_CODE void crash_dump_cli_init(void)
{
    //! register the crash dump commands
    FpFwCliRegisterTable(s_cd_cmd_list, FPFW_ARRAY_SIZE(s_cd_cmd_list));
}

static PLACED_CODE FPFW_CLI_STATUS cd_register_beef(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);
    crash_dump_register_address32(&dead_beef, sizeof(dead_beef), FPFW_CD_DUMP_PRIORITY_NORMAL);
    crash_dump_register_address32(&beef_cafe, sizeof(beef_cafe), FPFW_CD_DUMP_PRIORITY_NORMAL);

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cd_register_string(int argc, const char** pp_argv)
{
    if (argc == 2)
    {
        const char* input_str = pp_argv[1];
        FpFwCliPrint("Storing %s\n", input_str);

        crash_dump_register_address32((void*)input_str, strlen(input_str), FPFW_CD_DUMP_PRIORITY_NORMAL);
    }
    else
    {
        FpFwCliPrint("Invalid arguments!\n");
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_CLI_INVALID_PARAMS, argc);
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cd_bug_check(int argc, const char** pp_argv)
{
    uint32_t crashCode = KNG_E_FAIL;

    if (argc == 2)
    {
        crashCode = atoi(pp_argv[1]);
        FpFwCliPrint("Using Crash Code: %d\n", crashCode);
    }
    else
    {
        FpFwCliPrint("Using Default Crash Code: %d\n", crashCode);
        CRASH_DUMP_ET_WARNING_PARAM(CRASH_DUMP_ET_TYPE_CLI_INVALID_PARAMS, crashCode);
    }

    FpFwCliPrint("Crash Dump collection requested. Entering dump handler\n");
    BUG_CHECK(crashCode, 2, 3);
    FpFwCliPrint("Crash Dump collection completed.\n");

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cd_trigger_exception(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);
    volatile uint32_t* const invalid_ptr = (volatile uint32_t* const)(0xFFFFFFFF);

    FpFwCliPrint("Triggering exception\n");
    FpFwCliPrint("Dereferencing invalid_ptr [%lu]\n", *invalid_ptr);
    CRASH_DUMP_ET_ERROR(CRASH_DUMP_ET_TYPE_CLI_EXCEPTION);

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cd_set_single_core_mode(int argc, const char** pp_argv)
{
    if (argc == 2)
    {
        crash_dump_context_t* ctx = crash_dump_context();

        ctx->single_core_mode = !!atoi(pp_argv[1]);
        FpFwCliPrint("Crash dump %s core mode\n", ctx->single_core_mode ? "single" : "multi");
    }
    else
    {
        FpFwCliPrint("Invalid arguments!\n");
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_CLI_INVALID_PARAMS, argc);
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"
static PLACED_CODE void overflow_recurse(uint32_t sleep_tick) // NOLINT
{
#ifndef _WIN32
    char buffer[512] = "overflow_recurse"; // Eat up stack space
    FpFwCliPrint("%s - local buffer[%p] thread stack[%p-%p] %s\n",
                 buffer,
                 buffer,
                 _tx_thread_current_ptr->tx_thread_stack_start,
                 _tx_thread_current_ptr->tx_thread_stack_end,
                 ((void*)buffer > _tx_thread_current_ptr->tx_thread_stack_start &&
                  (void*)buffer < _tx_thread_current_ptr->tx_thread_stack_end)
                     ? ""
                     : "Stack overflow");

    // Simulate thread context switching to start ThreadX stack check.
    tx_thread_sleep(sleep_tick);
#endif

    overflow_recurse(sleep_tick);
}
#pragma GCC diagnostic pop

static PLACED_CODE FPFW_CLI_STATUS cd_stack_overflow(int argc, const char** pp_argv)
{
    uint32_t suspend_time = 1;

    if (argc == 2)
    {
        suspend_time = atoi(pp_argv[1]);

        // Zero suspend time can cause hard fault by memory corruption.
        FpFwCliPrint("Stack overflow using suspending for %d ticks\n", suspend_time);
    }
    else
    {
        FpFwCliPrint("Stack overflow using default suspend time %d ticks\n", suspend_time);
        CRASH_DUMP_ET_WARNING_PARAM(CRASH_DUMP_ET_TYPE_CLI_INVALID_PARAMS, suspend_time);
    }

    FpFwCliPrint("System Stack range [0x%08lx - 0x%08lx]\n", &__stack_start__, &__stack_end__);

    overflow_recurse(suspend_time);

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cd_get_status(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    crash_dump_dump_status(NULL);

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cd_set_status(int argc, const char** pp_argv)
{
    if (argc == 3)
    {
        crash_dump_type_t type = (crash_dump_type_t)atoi(pp_argv[1]);
        crash_dump_state_t state = (crash_dump_state_t)atoi(pp_argv[2]);

        if (type == CRASH_DUMP_TYPE_MINI || type == CRASH_DUMP_TYPE_FULL)
        {
            crash_dump_context_t* ctx = crash_dump_context();
            crash_dump_type_context_t* type_context = ctx->type_ctx[type];

            wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
            type_context->header->status = (uint16_t)state;
            release_semaphore(type_context->semaphore.id);

            FpFwCliPrint("Setting crash dump header status to %d\n", state);
        }
        else
        {
            FpFwCliPrint("Invalid type %d\n", type);
            return CLI_ERROR;
        }
    }
    else
    {
        FpFwCliPrint("Invalid usage!\n");
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static PLACED_CODE FPFW_CLI_STATUS cd_set_core_status(int argc, const char** pp_argv)
{
    if (argc == 4)
    {
        crash_dump_type_t type = (crash_dump_type_t)atoi(pp_argv[1]);
        uint16_t core = (uint16_t)atoi(pp_argv[2]);
        crash_dump_core_state_t core_state = (crash_dump_core_state_t)atoi(pp_argv[3]);

        if ((type == CRASH_DUMP_TYPE_MINI || type == CRASH_DUMP_TYPE_FULL) && (core < CRASH_DUMP_CORE_NUM * 2))
        {
            crash_dump_context_t* ctx = crash_dump_context();
            crash_dump_type_context_t* type_context = ctx->type_ctx[type];

            wait_for_semaphore(type_context->semaphore.id, type_context->semaphore.key);
            type_context->header->cores[core] = (uint8_t)core_state;
            release_semaphore(type_context->semaphore.id);
        }
        else
        {
            FpFwCliPrint("Invalid type %d, core %d, state %d\n", type, core, core_state);
            return CLI_ERROR;
        }
    }
    else
    {
        FpFwCliPrint("Invalid usage!\n");
        return CLI_ERROR;
    }

    return CLI_SUCCESS;
}

static FPFW_CLI_STATUS cd_transfer_dump(int argc, const char** pp_argv)
{
    FPFW_UNUSED(argc);
    FPFW_UNUSED(pp_argv);

    crash_dump_request_transfer_dump();

    return CLI_SUCCESS;
}