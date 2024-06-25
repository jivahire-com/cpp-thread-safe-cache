//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hw_version_init.cpp
 * Tests the init of the hw version component
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_hw_ver;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_idhw_set_sid_base(uintptr_t sid_base)
{
    FPFW_UNUSED(sid_base);
    function_called();
}

void __wrap_idsw_set_cpu_type(KNG_CPU_TYPE cpu_type)
{
    FPFW_UNUSED(cpu_type);
    function_called();
}

uint32_t __wrap_idhw_get_soc_id()
{
    return mock_type(uint32_t);
}
KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_idsw_set_die_id(KNG_DIE_ID die_id)
{
    FPFW_UNUSED(die_id);
    function_called();
}

KNG_PLAT_ID __wrap_idhw_get_platform_id_from_hw()
{
    return mock_type(KNG_PLAT_ID);
}

void __wrap_idsw_set_platform_sdv(KNG_PLAT_ID plat_id)
{
    FPFW_UNUSED(plat_id);
    function_called();
}

//
// Tests
//

TEST_FUNCTION(test_hw_ver_scp_init, nullptr, nullptr)
{
    // Set up expectations
    expect_function_call(__wrap_idhw_set_sid_base);
    expect_function_call(__wrap_idsw_set_cpu_type);

    will_return(__wrap_idhw_get_soc_id, 0);
    will_return(__wrap_idhw_get_die_id, DIE_0);

    expect_function_call(__wrap_idsw_set_die_id);

    will_return(__wrap_idhw_get_platform_id_from_hw, PLATFORM_UNDEFINED);

    expect_function_call(__wrap_idsw_set_platform_sdv);

    // Call API under test
    _fpfw_component_hw_ver.init_fn();
}
}
