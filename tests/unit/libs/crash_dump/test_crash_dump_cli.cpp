//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump_cli.cpp
 * Crash dump CLI tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, expect_function_...
#include <cstdint>         // for uint32_t, uint64_t
#ifdef _WIN32
    #include <excpt.h>
#endif

extern "C" {
#include <FpFwCli.h>    // for FPFW_CLI_COMMAND, FpFwCliRegisterTable
#include <crash_dump.h> // for crash_dump_cli_init
#include <stdarg.h>     // for va_list, va_start, va_end
#include <stdio.h>      // for printf

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static FPFW_CLI_COMMAND* s_cd_cmd_list = NULL;
static size_t s_cd_cmd_list_size = 0;
extern jmp_buf cd_test_setjmp_context;

/*------------- Functions ----------------*/
//
// Utility functions
//
static CLI_COMMAND_FN get_command_handler(const char* command)
{
    assert_non_null(s_cd_cmd_list);
    assert_true(s_cd_cmd_list_size > 0);

    if (command == nullptr)
    {
        return nullptr;
    }

    for (size_t i = 0; i < s_cd_cmd_list_size; i++)
    {
        if (strcmp(command, s_cd_cmd_list[i].Syntax) == 0)
        {
            return s_cd_cmd_list[i].Routine;
        }
    }

    return nullptr;
}

static int test_setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    expect_function_call(__wrap_FpFwCliRegisterTable);
    crash_dump_cli_init(); // Register the crash dump commands

    return 0;
}

static int test_teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    s_cd_cmd_list = NULL;
    s_cd_cmd_list_size = 0;

    return 0;
}

//
// Mocks
//
void __wrap_FpFwCliRegisterTable(PFPFW_CLI_COMMAND pTable, size_t TableLength)
{
    assert_non_null(pTable);
    assert_true(TableLength > 0);

    for (size_t i = 0; i < TableLength; i++)
    {
        assert_non_null(&pTable[i]);
        assert_non_null(pTable[i].MenuName);
        assert_non_null(pTable[i].Syntax);
        assert_non_null(pTable[i].Routine);
        assert_non_null(pTable[i].Description);
        assert_non_null(pTable[i].Usage);
    }

    s_cd_cmd_list = pTable;
    s_cd_cmd_list_size = TableLength;

    function_called();
}

void __wrap_FpFwCliPrint(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

bool __wrap_FPFwCDRegisterAddress32(void* address, uint32_t size)
{
    check_expected_ptr(address);
    check_expected(size);

    function_called();
    return true;
}

//
// Tests
//
TEST_FUNCTION(test_crash_dump_cli_init, nullptr, nullptr)
{
    expect_function_call(__wrap_FpFwCliRegisterTable);

    crash_dump_cli_init();
}

TEST_FUNCTION(test_cli_cd_register_beef, test_setup, test_teardown)
{
    int argc = 1;
    const char* argv[] = {"cd_register_beef"};
    uint32_t dead_beef = 0xDEADBEEF;
    uint32_t beef_cafe = 0xBEEFCAFE;

    // Get CLI handler
    CLI_COMMAND_FN handler = get_command_handler("cd_register_beef");
    assert_non_null(handler);

    // Setup expectations
    expect_memory(__wrap_FPFwCDRegisterAddress32, address, &dead_beef, sizeof(uint32_t));
    expect_value(__wrap_FPFwCDRegisterAddress32, size, sizeof(uint32_t));
    expect_function_call(__wrap_FPFwCDRegisterAddress32);
    expect_memory(__wrap_FPFwCDRegisterAddress32, address, &beef_cafe, sizeof(uint32_t));
    expect_value(__wrap_FPFwCDRegisterAddress32, size, sizeof(uint32_t));
    expect_function_call(__wrap_FPFwCDRegisterAddress32);

    // Call the CLI handler
    FPFW_CLI_STATUS status = handler(argc, argv);
    assert_true(status == CLI_SUCCESS);
}

TEST_FUNCTION(test_cli_cd_register_string, test_setup, test_teardown)
{
    int argc = 2;
    const char* argv[] = {"cd_register_string", "test_string"};

    // Get CLI handler
    CLI_COMMAND_FN handler = get_command_handler("cd_register_string");
    assert_non_null(handler);

    // Setup expectations
    expect_value(__wrap_FPFwCDRegisterAddress32, address, argv[1]);
    expect_value(__wrap_FPFwCDRegisterAddress32, size, 11);
    expect_function_call(__wrap_FPFwCDRegisterAddress32);

    // Call the CLI handler
    FPFW_CLI_STATUS status = handler(argc, argv);
    assert_true(status == CLI_SUCCESS);
}

TEST_FUNCTION(test_cli_cd_bug_check, test_setup, test_teardown)
{
    int argc = 2;
    const char* argv[] = {"cd_bug_check", "0x80000000"};

    // Get CLI handler
    CLI_COMMAND_FN handler = get_command_handler("cd_bug_check");
    assert_non_null(handler);

    // Call the CLI handler
    if (!setjmp(cd_test_setjmp_context))
    {
        handler(argc, argv);
        assert_true(false); // Should not reach here
    }
}

TEST_FUNCTION(test_cli_cd_set_single_core_mode, test_setup, test_teardown)
{
    int argc = 1;
    const char* argv[] = {"cd_set_single_core_mode"};

    // Get CLI handler
    CLI_COMMAND_FN handler = get_command_handler("cd_set_single_core_mode");
    assert_non_null(handler);

    // Call the CLI handler
    FPFW_CLI_STATUS status = handler(argc, argv);
    assert_true(status == CLI_SUCCESS);
}
}

TEST_FUNCTION(test_cli_cd_trigger_exception, test_setup, test_teardown)
{
    int argc = 1;
    const char* argv[] = {"cd_trigger_exception"};

    // Get CLI handler
    CLI_COMMAND_FN handler = get_command_handler("cd_trigger_exception");
    assert_non_null(handler);

#ifdef _WIN32
    // Call the CLI handler with catching exception.
    __try
    {
        handler(argc, argv);
        assert_true(false); // Should not reach here
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // Expected exception
        assert_true(GetExceptionCode() == 0xC0000005); // EXCEPTION_ACCESS_VIOLATION
        printf("Exception caught (0x%08lx)\n", GetExceptionCode());
    }
#endif
}

TEST_FUNCTION(test_cli_cd_stack_overflow, test_setup, test_teardown)
{
    int argc = 1;
    const char* argv[] = {"cd_stack_overflow"};

    // Get CLI handler
    CLI_COMMAND_FN handler = get_command_handler("cd_stack_overflow");
    assert_non_null(handler);

#ifdef _WIN32
    // Call the CLI handler with catching exception.
    __try
    {
        handler(argc, argv);
        assert_true(false); // Should not reach here
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // Expected exception
        // GetExceptionCode();
        assert_true(GetExceptionCode() == 0xC00000fd); // EXCEPTION_STACK_OVERFLOW
        printf("Exception caught (0x%08lx)\n", GetExceptionCode());
    }
#endif
}