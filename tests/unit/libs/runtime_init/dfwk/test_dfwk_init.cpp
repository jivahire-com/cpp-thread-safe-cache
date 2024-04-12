//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_dfwk_init.cpp
 * DFWK init tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <DfwkThreadXHost.h>
#include <FpFwUtils.h>
#include <fpfw_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_dfwk;

/*------------- Functions ----------------*/
//
// Mocks
//
uint32_t __wrap_DfwkThreadxHostInitialize(PDFWK_THREADX_HOST ThreadxHost,
                                          VOID* StackStart,
                                          ULONG StackSize,
                                          UINT Priority,
                                          UINT PreemptThreshold,
                                          ULONG TimeSlice)
{
    FPFW_UNUSED(ThreadxHost);
    FPFW_UNUSED(StackStart);
    FPFW_UNUSED(StackSize);
    FPFW_UNUSED(Priority);
    FPFW_UNUSED(PreemptThreshold);
    FPFW_UNUSED(TimeSlice);
    return mock_type(uint32_t);
}

TEST_FUNCTION(test_css_post_mesh_init, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_DfwkThreadxHostInitialize, 0);

    // Call API under test
    _fpfw_component_dfwk.init_fn();
}
}
