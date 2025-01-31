//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump_init.cpp
 * Crash dump init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <crash_dump.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_error.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern void get_crash_dump_mem_heap_pool(uint64_t* mem_pool_addr, uint32_t* size);
extern bool in_memory(uintptr_t start_addr, uintptr_t end_addr);

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_cd_init;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_crash_dump_init(crash_dump_config_t* config)
{
    check_expected_ptr(config);
    check_expected(config->die_index);
    check_expected(config->core_index);
    check_expected(config->is_primary);

    function_called();
}

void __wrap_crash_dump_init_post_mesh()
{
    function_called();
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    function_called();

    return mock_type(bool);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type()
{
    return mock_type(idsw_cpu_type_t);
}

idsw_plat_id_t __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(idsw_plat_id_t);
}

int32_t __wrap_exception_handler_init(void)
{
    function_called();

    return mock_type(int32_t);
}

//
// Tests
//
TEST_FUNCTION(test_crash_dump_init_in_memory, nullptr, nullptr)
{
    // Test valid address and size
    uintptr_t start_addr = (uintptr_t)0x40;
    uintptr_t end_addr = start_addr + 8 - 1;

    assert_true(in_memory(start_addr, end_addr));

    // Test invalid address
    start_addr = (uintptr_t)0xFFFFFFF0;
    end_addr = start_addr + 6 - 1;

    assert_false(in_memory(start_addr, end_addr));
}

TEST_FUNCTION(test_crash_dump_init, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_idsw_get_die_id, DIE_0);
#if defined(SCP_RUNTIME_INIT)
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
#elif defined(MCP_RUNTIME_INIT)
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
#endif
    expect_any(__wrap_crash_dump_init, config);
    expect_value(__wrap_crash_dump_init, config->die_index, DIE_0);
#if defined(SCP_RUNTIME_INIT)
    expect_value(__wrap_crash_dump_init, config->core_index, CRASH_DUMP_CORE_SCP);
    expect_value(__wrap_crash_dump_init, config->is_primary, false);
#elif defined(MCP_RUNTIME_INIT)
    expect_value(__wrap_crash_dump_init, config->core_index, CRASH_DUMP_CORE_MCP);
    expect_value(__wrap_crash_dump_init, config->is_primary, true);
#endif

    expect_function_call(__wrap_crash_dump_init);

    will_return(__wrap_exception_handler_init, KNG_SUCCESS);
    expect_function_call(__wrap_exception_handler_init);

#if defined(SCP_RUNTIME_INIT)
    will_return(__wrap_idhw_is_single_die_boot_en, false);
    expect_function_call(__wrap_idhw_is_single_die_boot_en);
#elif defined(MCP_RUNTIME_INIT)
#endif

    // Check dependencies
    assert_string_equal("hw_ver", _fpfw_component_cd_init.children[0]);
    assert_string_equal("gpio_lib", _fpfw_component_cd_init.children[1]);

    // Call API under test
    _fpfw_component_cd_init.init_fn();
}
}
