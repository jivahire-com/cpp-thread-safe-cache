//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddrss.cpp
 * ddrss tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h>
#include <ddrss.h>
#include <ddrss_lib.h>
#include <idsw.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// Mocks
//

int __wrap_ddrss_init(ddrss_cfg_knobs_t* cfg_knobs)
{
    FPFW_UNUSED(cfg_knobs);
    return mock_type(int);
}

//
// Tests
//

TEST_FUNCTION(test_ddrss_lib_init_skip, NULL, NULL)
{

    const uint8_t test_die = (DIE_ID)0;

    // ddrss init is skipped on svp, therefore no expectations
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    ddrss_lib_init(test_die);

    // ddrss init is skipped if not on an FPGA, therefore no expectations
    idsw_set_platform_sdv(PLATFORM_UNDEFINED);
    ddrss_lib_init(test_die);
}

TEST_FUNCTION(test_ddrss_lib_init_fpga, NULL, NULL)
{

    const uint8_t test_die = (DIE_ID)0;

    // ddrss init is not skipped on the Big FPGA, and the
    // atu map is fixed so no atu map / un map calls
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);

    will_return(__wrap_ddrss_init, SILIBS_SUCCESS);

    ddrss_lib_init(test_die);
}
}