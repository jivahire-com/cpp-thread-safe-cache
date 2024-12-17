//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddrss_init.cpp
 * DDRSS init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <FpFwUtils.h>   // for FPFW_UNUSED
#include <ddr_manager.h> // for ddr_manager_init, ddr_service_context_t
#include <ddrss.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_ddr;

/*------------- Functions ----------------*/

//
// Mocks
//

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

KNG_DIE_ID __wrap_idsw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

KNG_DIE_ID __wrap_idhw_get_die_id()
{
    return mock_type(KNG_DIE_ID);
}

void __wrap_ddr_manager_init(ddr_service_context_t* pddr_service_ctx, ddr_service_config_t* pconfig, fpfw_icc_base_ctx_t* icc_ctx)
{
    FPFW_UNUSED(pddr_service_ctx);
    FPFW_UNUSED(pconfig);
    FPFW_UNUSED(icc_ctx);
}

uint32_t __wrap_FPFwCoreInterruptEnableVector(uint32_t interrupt_id)
{
    FPFW_UNUSED(interrupt_id);
    return 0;
}

uint32_t __wrap_FPFwCoreInterruptRegisterCallback(uint32_t interrupt_id)
{
    FPFW_UNUSED(interrupt_id);
    return 0;
}

//
// Tests
//
TEST_FUNCTION(test_ddr_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idhw_get_die_id, test_die);

    // Call API under test
    _fpfw_component_ddr.init_fn();
}

TEST_FUNCTION(test_prod_ddrss_lib_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;
    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // Call API under test
    prod_ddrss_lib_init(test_die);
}

} // extern "C"
