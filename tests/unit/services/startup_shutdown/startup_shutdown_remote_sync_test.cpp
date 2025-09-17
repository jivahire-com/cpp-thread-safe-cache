//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file startup_shutdown_thread_test.cpp
 * Startup shutdown service thread tests
 */

/*------------- Includes -----------------*/
#include "sos_test.h" // for POWER_TEST

#include <cstddef> // for NULL
#include <cstdint> // for uintptr_t

extern "C" {

#include <CMockaWrapper.h> // for check_expected, check_expected_ptr
#include <DfwkCommon.h>    // for DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE
#include <FpFwLinkedList.h>
#include <FpFwUtils.h> // for FPFW_ARRAY_SIZE
#include <atu_api.h>
#include <icc_platform_defines.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_scp_tfa_shared.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <startup_shutdown.h>     // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_i.h>   // for sos_queue_start_phase, sos_thread_...
#include <startup_shutdown_ssi.h> // for _ssi_startup_stage, _startup_type
#include <string.h>

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
#ifndef D1_ARSM_MSCP_D2D_SYNC_POINT_BASE
    #define D1_ARSM_MSCP_D2D_SYNC_POINT_SIZE (8)
    #define D1_ARSM_MSCP_D2D_SYNC_POINT_BASE (D1_ARSM_MSCP_D2D_POWER_DATA_END + 1)
    #define D1_ARSM_MSCP_D2D_SYNC_POINT_END \
        (D1_ARSM_MSCP_D2D_SYNC_POINT_BASE + D1_ARSM_MSCP_D2D_SYNC_POINT_SIZE - 1)
#endif

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
extern "C" {

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

uint32_t __wrap_mmio_read32(volatile uint32_t* addr)
{
    check_expected(addr);
    function_called();
    return mock_type(uint32_t);
}

void __wrap_mmio_write32(volatile uint32_t* addr, uint32_t data)
{
    check_expected(addr);
    check_expected(data);
    function_called();
}

} // extern "C"

// test for wait_for_remote_die_boot_stage
SOS_TEST(wait_for_remote_die_boot_stage_single_die, NULL, NULL)
{
    int status = 0;
    will_return(__wrap_idhw_is_single_die_boot_en, true);

    // call the function
    startup_shutdown_boot_stage_t test_stage = {STARTUP_MCP_LOAD, STARTUP_MCP_LOAD, 0, false, false};

    status = wait_for_remote_die_boot_stage(test_stage);

    assert_true(status);
}

SOS_TEST(wait_for_remote_die_boot_stage_dual_die, NULL, NULL)
{
    int status = 0;
    startup_shutdown_boot_stage_t test_stage = {STARTUP_MCP_LOAD, STARTUP_MCP_LOAD, 0, false, true};

    will_return(__wrap_idhw_is_single_die_boot_en, false);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    uint32_t die0_point_base =
        MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR + ARSM_GET_REGION_OFFSET(D1_ARSM_MSCP_D2D_SYNC_POINT_BASE);

    uint32_t die1_point_base = die0_point_base + sizeof(uint32_t);

    //! Will synchronize with the remote die for dual die boot
    expect_value(__wrap_mmio_write32, addr, die0_point_base);
    expect_value(__wrap_mmio_write32, data, ((uint32_t)RMSS_D2D_SPI_SYNC_ENUM_START | (uint32_t)test_stage.stage));
    expect_function_call(__wrap_mmio_write32);

    expect_value(__wrap_mmio_read32, addr, die1_point_base);
    will_return(__wrap_mmio_read32, ((uint32_t)RMSS_D2D_SPI_SYNC_ENUM_START | (uint32_t)test_stage.stage));
    expect_function_call(__wrap_mmio_read32);
    //! call the function
    status = wait_for_remote_die_boot_stage(test_stage);

    assert_true(status);
}