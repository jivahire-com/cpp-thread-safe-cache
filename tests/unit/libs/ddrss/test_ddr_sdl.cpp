//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_ddr_sdl.cpp
 * Tests Shared Defect List (SDL) related functions
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_non_null, CmockaWrapperTest, TEST_...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <ddr_atu_map.h>
#include <ddr_manager.h>
#include <ddrss_reserved_regions.h>
#include <ddrss_sdl.h>
#include <fpfw_cfg_mgr.h>
#include <idsw.h>
#include <kng_error.h>
#include <mscp_exp_rmss_memory_map.h>
#include <stddef.h> // for NULL, size_t
// #include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
uintptr_t __real_ddrss_atu_map_cfg_space(uint32_t die_num);
void __real_ddrss_atu_unmap_cfg_space(uint32_t die_num);

// Test fixtures
static uint8_t s_sdl_payload[SCP_EXP_SDL_LOAD_SIZE] = {0};
static MEMORY_DEFECT_LIST_HEADER s_empty_header = {0};

/*------------- Functions ----------------*/
//
// Tests
//
TEST_FUNCTION(test_sdl_load_success, NULL, NULL)
{
    // Set up die id for test
    idsw_set_die_id(DIE_0);

    // ATU mapping seeds the mapped start address and returns success
    will_return(__wrap_atu_map, (uint32_t)0x01234567);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    // SDL load path
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)s_sdl_payload);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);
    will_return(__wrap_variable_service_sync_get_variable, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    load_shared_defect_list_to_DDR();
}

TEST_FUNCTION(test_sdl_load_not_found_creates_empty, NULL, NULL)
{
    const uint32_t EXPECTED_CHECKSUM = 0x1737; // Pre-calculated checksum:

    // Set up die id for test
    idsw_set_die_id(DIE_0);

    // ATU mapping seeds destination base and succeeds
    will_return(__wrap_atu_map, (uint32_t)(uintptr_t)&s_empty_header);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    // SDL load attempts and reports not found
    will_return(__wrap_sdl_get_mem_ctx, (uintptr_t)s_sdl_payload);
    will_return(__wrap_sdl_get_mem_ctx, SCP_EXP_SDL_LOAD_SIZE);

    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);
    will_return(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    load_shared_defect_list_to_DDR();

    assert_int_equal(s_empty_header.Signature, (uint32_t)PSHED_PI_DEFECT_LIST_SIGNATURE);
    assert_int_equal(s_empty_header.DefectCount, 0);
    assert_int_equal(s_empty_header.Length, sizeof(MEMORY_DEFECT_LIST_HEADER));
    assert_int_equal(s_empty_header.Version, MEMORY_DEFECT_VERSION_20);
    assert_int_equal(s_empty_header.Checksum, EXPECTED_CHECKSUM);
}

TEST_FUNCTION(test_sdl_load_skips_die1, NULL, NULL)
{
    // Set up die id for test
    idsw_set_die_id(DIE_1);

    load_shared_defect_list_to_DDR();
}

TEST_FUNCTION(test_sdl_map_and_get_atu_start_addr, NULL, NULL)
{
    // Seed ATU map to set start address
    will_return(__wrap_atu_map, (uint32_t)0xDEADBEEF);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    sdl_map_atu(SDL_RESERVATION_BASE);

    uintptr_t start_addr = sdl_get_atu_start_addr();
    assert_int_equal(start_addr, 0xDEADBEEF);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    sdl_unmap_atu();
}
} // extern "C"