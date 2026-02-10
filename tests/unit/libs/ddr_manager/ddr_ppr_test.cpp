//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_test.cpp
 * DDR Manager tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep
#include <stdio.h>
#include <string.h> // IWYU pragma: keep

extern "C" {
#include "ddr_mocks.h"
#include "memory_map/ddrss_reserved_regions.h"

#include <atu_lib.h>     // for atu_map
#include <bdat_schema.h> // for bdat structure
#include <cli_ddr.h>
#include <crash_dump.h> // for crash_dump_init
#include <ddr_i3c.h>
#include <ddr_manager.h> // for ddr_manager_init, ddr_service_context_t
#include <ddr_manager_bwl.h>
#include <ddr_manager_dfwk.h>
#include <ddr_manager_i.h> // for ddr_poll_dimms, ddr_worker_thread_func
#include <ddr_ppr.h>
#include <ddr_rhtlm_service.h>
#include <ddrss_lib.h>
#include <ddrss_sdl.h>
#include <error_handler.h> // for set_error_handler_return
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h> // for fpfw_init_result_t, fpfw_init_component_t
#include <hsp_firmware_headers.h>
#include <idsw_kng.h>
#include <kingsgate_hsp_mailbox_commands.h>
#include <mscp_exp_rmss_memory_map.h>
#include <mscp_exp_spi_synchronize_dies.h>
#include <sensor_fifo_service.h>
#include <smbios_structs.h>
#include <startup_shutdown.h>
#include <thread_x_mocks.h>
#include <tx_api.h> // for TX_SUCCESS, ULONG, TX_NOT_DONE, TX_NO_MEMORY

} // extern "C"

/*-- Symbolic Constant Macros (defines) --*/
#define UNUSED(x) (void)(x)

// #define DEBUG_PRINT_EXPECT_CALLS_ENABLED (true)
#ifdef DEBUG_PRINT_EXPECT_CALLS_ENABLED
    #define PRINT_EXPECT_CALL(fn, param, ret) printf("Expecting %s(%s) -> %d\n", #fn, #param, ret)
#else
    #define PRINT_EXPECT_CALL(fn, param, ret) (void)0
#endif

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

uint8_t ppr_test_buffer[D0_ARSM_SDL_RESERVED_SIZE + SDL_MAX_SIZE] = {0};

/*------------- Functions ----------------*/
//
// Mocks
//

extern bool g_ppr_testing;

//
// Tests
//

/////////////////////////////////////////////////////////////////////
//                       PPR Related Tests                         //
/////////////////////////////////////////////////////////////////////
TEST_FUNCTION(ppr_setup_no_ppr_var_DIE_0_cold_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D0;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    // Reuse local test framework buffer as stand-in for arsm0
    will_return_count(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0], 2);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- d0_determine_ppr_type (runs before die sync) --
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // Variable not found handled same as PPR_NONE
    will_return(__wrap_variable_service_sync_get_variable, DDRSS_PPR_NONE);
    will_return(__wrap_variable_service_sync_get_variable, KNG_E_NOT_FOUND);

    // send_ppr_type_to_die1 (runs before die sync)
    expect_value(__wrap_send_ppr_type_to_die1, ppr_type, DDRSS_PPR_NONE);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // Final synchronize dies before ddr_init
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_no_ppr_var_DIE_1_cold_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D1;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    will_return(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0]);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // -- handle_die1_ppr_reception (runs after die sync) --
    will_return(__wrap_receive_ppr_type_from_die0, DDRSS_PPR_NONE);

    // Final synchronize dies before ddr_init
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_hppr_var_DIE_0_cold_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D0;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    will_return_count(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0], 2);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- d0_determine_ppr_type (runs before die sync) --
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // Variable found: DDRSS_HPPR
    will_return(__wrap_variable_service_sync_get_variable, DDRSS_HPPR);
    will_return(__wrap_variable_service_sync_get_variable, KNG_SUCCESS);

    // Reset PPR variable since ppr_type != DDRSS_PPR_NONE
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // send_ppr_type_to_die1 (runs before die sync)
    expect_value(__wrap_send_ppr_type_to_die1, ppr_type, DDRSS_HPPR);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // Final sync before ddrss_init
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_hppr_var_DIE_1_cold_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D1;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    will_return(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0]);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // -- handle_die1_ppr_reception (runs after die sync) --
    will_return(__wrap_receive_ppr_type_from_die0, DDRSS_HPPR);

    // -- synchronize_dies_before_init --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_hppr_var_DIE_0_warm_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D0;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_WARM;

    // -- init_ppr_shared_memory_arsm0 --
    will_return_count(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0], 2);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- d0_determine_ppr_type (runs before die sync) --
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // Variable found: DDRSS_HPPR but warm reset so PPR skipped
    will_return(__wrap_variable_service_sync_get_variable, DDRSS_HPPR);
    will_return(__wrap_variable_service_sync_get_variable, KNG_SUCCESS);

    // send_ppr_type_to_die1 - sends NONE due to warm reset (runs before die sync)
    expect_value(__wrap_send_ppr_type_to_die1, ppr_type, DDRSS_PPR_NONE);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // Final sync before ddrss_init
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_sppr_var_DIE_0_cold_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D0;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    will_return_count(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0], 2);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- d0_determine_ppr_type (runs before die sync) --
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // Variable found: DDRSS_SPPR
    will_return(__wrap_variable_service_sync_get_variable, DDRSS_SPPR);
    will_return(__wrap_variable_service_sync_get_variable, KNG_SUCCESS);

    // Reset PPR variable since ppr_type != DDRSS_PPR_NONE
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // send_ppr_type_to_die1 (runs before die sync)
    expect_value(__wrap_send_ppr_type_to_die1, ppr_type, DDRSS_SPPR);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // Final sync before ddrss_init
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_mppr_var_DIE_0_cold_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D0;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    will_return_count(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0], 2);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- d0_determine_ppr_type (runs before die sync) --
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // Variable found: DDRSS_MPPR
    will_return(__wrap_variable_service_sync_get_variable, DDRSS_MPPR);
    will_return(__wrap_variable_service_sync_get_variable, KNG_SUCCESS);

    // Reset PPR variable since ppr_type != DDRSS_PPR_NONE
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // send_ppr_type_to_die1 (runs before die sync)
    expect_value(__wrap_send_ppr_type_to_die1, ppr_type, DDRSS_MPPR);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // Final sync before ddrss_init
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_hppr_var_DIE_0_single_die_boot, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D0;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // Single die boot - skips die sync
    will_return(__wrap_idhw_is_single_die_boot_en, true);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_invalid_ppr_type_DIE_0_cold_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D0;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    will_return_count(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0], 2);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- d0_determine_ppr_type (runs before die sync) --
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // Variable found with invalid type (value > DDRSS_MPPR)
    will_return(__wrap_variable_service_sync_get_variable, (DDRSS_PPR_TYPE)(DDRSS_MPPR + 1));
    will_return(__wrap_variable_service_sync_get_variable, KNG_SUCCESS);

    // send_ppr_type_to_die1 - sends NONE due to invalid type (runs before die sync)
    expect_value(__wrap_send_ppr_type_to_die1, ppr_type, DDRSS_PPR_NONE);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // Final sync before ddrss_init
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_variable_service_error_DIE_0, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D0;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    will_return_count(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0], 2);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- d0_determine_ppr_type (runs before die sync) --
    will_return(__wrap_variable_service_initialize_ctx, SILIBS_SUCCESS);

    // Variable service returns error (not KNG_SUCCESS or KNG_E_NOT_FOUND)
    will_return(__wrap_variable_service_sync_get_variable, DDRSS_PPR_NONE);
    will_return(__wrap_variable_service_sync_get_variable, KNG_E_ABORT);

    // send_ppr_type_to_die1 - sends NONE due to error (runs before die sync)
    expect_value(__wrap_send_ppr_type_to_die1, ppr_type, DDRSS_PPR_NONE);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // Final sync before ddrss_init
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_0);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_sppr_var_DIE_1_cold_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D1;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    will_return(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0]);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // -- handle_die1_ppr_reception --
    will_return(__wrap_receive_ppr_type_from_die0, DDRSS_SPPR);

    // -- synchronize_dies_before_init --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}

TEST_FUNCTION(ppr_setup_mppr_var_DIE_1_cold_reset, NULL, NULL)
{
    ddrss_cfg_knobs_t test_ddrss_knobs = {};
    ddrss_get_config(&test_ddrss_knobs);

    test_ddrss_knobs.die_id = SOC_D1;
    test_ddrss_knobs.reset_reason = DDRSS_SYS_RESET_COLD;

    // -- init_ppr_shared_memory_arsm0 --
    will_return(__wrap_get_sdl_arsm0_addr, &ppr_test_buffer[0]);

    will_return(__wrap_idhw_is_single_die_boot_en, false);

    // -- synchronize_dies_for_ppr --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    // -- handle_die1_ppr_reception --
    will_return(__wrap_receive_ppr_type_from_die0, DDRSS_MPPR);

    // -- synchronize_dies_before_init --
    expect_value(__wrap_mscp_exp_spi_synchronize_dies_timeout, die_id, DIE_1);
    will_return(__wrap_mscp_exp_spi_synchronize_dies_timeout, SILIBS_SUCCESS);

    g_ppr_testing = true;
    ppr_setup(&test_ddrss_knobs);
    g_ppr_testing = false;
}
