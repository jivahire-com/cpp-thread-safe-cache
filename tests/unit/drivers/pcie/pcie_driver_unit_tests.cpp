//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pcie_driver_unit_tests.cpp
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <atu_lib.h>
#include <cmocka.h> // IWYU pragma: keep
#include <cstddef>  // IWYU pragma: keep
#include <cstdint>  // IWYU pragma: keep
#include <fpfw_cfg_mgr.h>
#include <idsw_kng.h>
#include <interrupts.h>
#include <pcie_common.h>
#include <pcie_ss_common.h>
#include <pciess_int.h>
#include <stdint.h>
#include <tx_port.h>

extern "C" {
#include "pcie_silibs_mocks.h"

#include <DfwkPtrTypes.h>
#include <cper.h>
#include <error_handler.h>
#include <kng_soc_constants.h>
#include <pcie_bdat_i.h>
#include <pcie_common.h>
#include <pcie_config_i.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <pcie_einj_structs.h>
#include <pcie_irq.h>
#include <pcie_rp_common.h>
#include <pcie_rp_ide.h>
#include <pcie_rp_rasdes.h>
#include <pcie_rp_vsecras.h>
#include <pcie_ss_common.h> // for pcie_ss_entity_t
#include <pciess_int.h>
#include <ras_common.h>
#include <silibs_status.h> // for SILIBS_E_PARAM, SILIBS_SUCCESS
#include <tx_api.h>

cxl_region_params_t __real_config_get_cxl_params_die0(void);
cxl_region_params_t __real_config_get_cxl_params_die1(void);
bool __real_config_get_pcie_configuration_mirroring(void);
}

/*-- Symbolic Constant Macros (defines) --*/
/*
 * This corresponds to:
 * Number of rpss per die (4) * Number of rps per rpss (4)
 * Upto 1 outstanding 1 req. per rp can be sent by the service.
 */
#define BUGCHECK_MOCK_RETURN    (setjmp(mock_jump_buf))
#define bugcheck_mock_return()  BUGCHECK_MOCK_RETURN
#define MAX_ASYNC_REQ_POOL_SIZE (16)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pciess_device_t dev;
static pciess_device_interface_t iface;
static pcie_async_request_t r;
static jmp_buf mock_jump_buf;
static bool should_return;
uint8_t mock_buf[2048];
static bool ift_enabled = false;

/* mock entity*/
pcie_ss_entity_t mock_pcie_ent;

/*------------- Functions ----------------*/
extern "C" {
void __wrap_crash_dump_bug_check(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(p0);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    function_called();

    /* Handle noreturn, allowing control to return to test */
    if (!should_return)
    {
        longjmp(mock_jump_buf, 1);
    }
}

bool __wrap_ift_is_enabled(void)
{
    return ift_enabled;
}
}

static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);
    return 0;
}

static int test_teardown(void** ctx)
{
    FPFW_UNUSED(ctx);
    /* Cleanup dispatch pool */
    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value_count(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT, -1);
    will_return_always(__wrap__txe_semaphore_get, TX_SUCCESS);
    expect_any_always(__wrap__txe_semaphore_put, semaphore_ptr);
    will_return_always(__wrap__txe_semaphore_put, TX_SUCCESS);
    for (uint8_t i = 0; i < NUM_RPSS; i++)
    {
        for (uint8_t ii = 0; ii < PCIESS_NUM_PORTS; ii++)
        {
            for (uint8_t iii = 0; iii < PCIE_MAX_ASYNC_REQ; iii++)
            {
                pcie_async_request_t* r = get_pending_async_req_for_this_rp(i, ii, (pcie_rp_async_request_t)iii);
                if (r != NULL)
                {
                    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r->header));
                    complete_async_req_for_this_rp(r);
                }
            }
        }
    }

    /* Clean up statics */
    memset(&dev, 0x0, sizeof(dev));
    memset(&r, 0x0, sizeof(r));
    memset(&iface, 0x0, sizeof(iface));
    return 0;
}

/* Test cases for top-level driver initialization */
TEST_FUNCTION(driver_init, NULL, NULL)
{
    auto* sched = (PDFWK_SCHEDULE)0xdeadbeef;

    /* Check whether bad inputs are rejected */
    expect_function_calls(__wrap_crash_dump_bug_check, 1);
    if (!bugcheck_mock_return())
    {
        pcie_dfwk_init(nullptr, nullptr);
    }

    expect_any_always(__wrap__txe_semaphore_create, semaphore_ptr);
    expect_any_always(__wrap__txe_semaphore_create, name_ptr);
    expect_any_always(__wrap__txe_semaphore_create, semaphore_control_block_size);
    will_return_always(__wrap__txe_semaphore_create, TX_SUCCESS);
    expect_value_count(__wrap__txe_semaphore_create, initial_count, 1, MAX_ASYNC_REQ_POOL_SIZE);
    pcie_dfwk_init(&dev, sched);
}

/* Test cases for top-level interface initialization */
TEST_FUNCTION(interface_init, test_setup, test_teardown)
{
    /* Check whether bad inputs are rejected */
    expect_function_calls(__wrap_crash_dump_bug_check, 1);
    if (!bugcheck_mock_return())
    {
        pcie_dfwk_interface_init(nullptr, nullptr);
    }

    /* No need to mock any dfwk functionality in the good case as of now */
    pcie_dfwk_interface_init(&dev, &iface);
    assert_ptr_equal(iface.dev, &dev);
}

/* Test case for initial pciess init sync. request for non-mirrored configurations successful path */
TEST_FUNCTION(test_pcie_rpss_init_soc1_success, test_setup, test_teardown)
{
    /*Setup the mock interface and device*/
    pcie_root_bridge_config rb_configs[4];
    pciess_device_t dev;
    dev.rb_configs = rb_configs;
    pciess_device_interface_t iface;
    iface.dev = &dev;

    /* Setup silibs expectations */
    /* Will always is used because IS_PLATFORM_FPGA() uses 3 calls to idsw_get_platform_sdv */
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    for (uint8_t i = 0; i < NUM_RPSS; i++)
    {
        /* Setup the request for an rpss */
        pcie_sync_request_t r;
        r.header.RequestType = INITIAL_CONFIG_REQUEST;
        r.req_type = INITIAL_CONFIG_REQUEST;
        r.rpss_index = (RPSS_INSTANCE)i;
        r.rp_index = 0;

        /* Set the owning interface*/
        auto req = (PDFWK_SYNC_REQUEST_HEADER)&r;
        req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

        mock_pcie_ent.id = r.rpss_index;
        will_return(__wrap_get_rpss_resolved_base, 0xDEADBEEF);
        expect_value(__wrap_pciess_get_entity, rpss_idx, i);
        will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
        will_return(__wrap_system_info_get_soc_position, 0x00);
        expect_value(__wrap_pciess_config_entity, program_phy_regs, true);
        expect_value(__wrap_pciess_config_entity, enable_apu, true);
        will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
        will_return(__wrap_pciess_config_ss_for_bifur, SILIBS_SUCCESS);
        will_return(__wrap_pciess_deassert_por_reset, SILIBS_SUCCESS);
        will_return(__wrap_pciess_phys_toggle_clocks, SILIBS_SUCCESS);
        will_return(__wrap_idsw_get_die_id, DIE_0);

        cxl_region_params_t cxl_region_params_die0 = __real_config_get_cxl_params_die0();
        will_return(__wrap_config_get_cxl_params_die0, &cxl_region_params_die0);

        int32_t ret = pcie_sched_sync_op(&(r.header));
        assert_int_equal(ret, 0);
        assert_int_equal(r.status, SILIBS_SUCCESS);
    }
}

/* Test case for initial pciess init sync. request for mirrored configurations successful path */
TEST_FUNCTION(test_pcie_rpss_init_soc2_success, test_setup, test_teardown)
{
    /*Setup the mock interface and device*/
    pcie_root_bridge_config rb_configs[4];
    pciess_device_t dev;
    dev.rb_configs = rb_configs;
    pciess_device_interface_t iface;
    iface.dev = &dev;

    /* Setup silibs expectations */
    /* Will always is used because IS_PLATFORM_FPGA() uses 3 calls to idsw_get_platform_sdv */
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    for (uint8_t i = 0; i < NUM_RPSS; i++)
    {
        /* Setup the request for an rpss */
        pcie_sync_request_t r;
        r.header.RequestType = INITIAL_CONFIG_REQUEST;
        r.req_type = INITIAL_CONFIG_REQUEST;
        r.rpss_index = (RPSS_INSTANCE)i;
        r.rp_index = 0;

        /* Set the owning interface*/
        auto req = (PDFWK_SYNC_REQUEST_HEADER)&r;
        req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

        mock_pcie_ent.id = r.rpss_index;
        will_return(__wrap_get_rpss_resolved_base, 0xDEADBEEF);
        expect_value(__wrap_pciess_get_entity, rpss_idx, i);
        will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
        will_return(__wrap_system_info_get_soc_position, 0x01);
        bool is_mirroring = __real_config_get_pcie_configuration_mirroring();
        is_mirroring = true;
        will_return(__wrap_config_get_pcie_configuration_mirroring, is_mirroring);
        expect_value(__wrap_pciess_config_entity, program_phy_regs, true);
        expect_value(__wrap_pciess_config_entity, enable_apu, true);
        will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
        will_return(__wrap_pciess_config_ss_for_bifur, SILIBS_SUCCESS);
        will_return(__wrap_pciess_deassert_por_reset, SILIBS_SUCCESS);
        will_return(__wrap_pciess_phys_toggle_clocks, SILIBS_SUCCESS);
        will_return(__wrap_idsw_get_die_id, DIE_0);

        cxl_region_params_t cxl_region_params_die0 = __real_config_get_cxl_params_die0();
        will_return(__wrap_config_get_cxl_params_die0, &cxl_region_params_die0);

        int32_t ret = pcie_sched_sync_op(&(r.header));
        assert_int_equal(ret, 0);
        assert_int_equal(r.status, SILIBS_SUCCESS);
    }
}

/* Test case for successful rb config population*/
TEST_FUNCTION(test_populate_rb_configs_from_rpss_entity, test_setup, test_teardown)
{
    /*Setup the mock interface and device*/
    pcie_root_bridge_config rb_configs[4];
    pciess_device_t dev;
    dev.rb_configs = rb_configs;
    pciess_device_interface_t iface;
    iface.dev = &dev;

    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = INITIAL_CONFIG_REQUEST;
    r.req_type = INITIAL_CONFIG_REQUEST;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    /* Set the owning interface*/
    PDFWK_SYNC_REQUEST_HEADER req = (PDFWK_SYNC_REQUEST_HEADER)&r;
    req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

    mock_pcie_ent.id = r.rpss_index;
    mock_pcie_ent.rps[0].enabled = true;
    mock_pcie_ent.rps[0].valid = true;
    mock_pcie_ent.rps[3].enabled = true;
    mock_pcie_ent.rps[3].valid = true;

    /* Setup silibs expectations */
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return(__wrap_get_rpss_resolved_base, 0xDEADBEEF);
    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return_always(__wrap_system_info_get_soc_position, 0x0);
    expect_value(__wrap_pciess_config_entity, program_phy_regs, false);
    expect_value(__wrap_pciess_config_entity, enable_apu, true);
    will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
    will_return(__wrap_pciess_config_ss_for_bifur, SILIBS_SUCCESS);
    will_return(__wrap_pciess_deassert_por_reset, SILIBS_SUCCESS);
    will_return(__wrap_pciess_phys_toggle_clocks, SILIBS_SUCCESS);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    cxl_region_params_t cxl_region_params_die0 = __real_config_get_cxl_params_die0();
    will_return(__wrap_config_get_cxl_params_die0, &cxl_region_params_die0);

    pcie_sched_sync_op(&(r.header));

    assert_int_equal(rb_configs[0].flags.is_enabled, true);
    assert_int_equal(rb_configs[3].flags.is_enabled, true);
    assert_int_equal(rb_configs[0].flags.is_secondary_soc, false);
}

TEST_FUNCTION(test_pcie_rpss_pre_rp_ready_init_success, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = PRE_RP_INIT_REQUEST;
    r.req_type = PRE_RP_INIT_REQUEST;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_pciess_poll_phys_sram_init_done, SILIBS_SUCCESS);
    will_return(__wrap_pciess_phys_program_fw, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_pre_rp_ready_init, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_pcie_rpss_pre_rp_ready_init_success_sram_bypass, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = PRE_RP_INIT_REQUEST;
    r.req_type = PRE_RP_INIT_REQUEST;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_pciess_rps_pre_rp_ready_init, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_pcie_rpss_post_rp_ready_init_success, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = POST_RP_INIT_REQUEST;
    r.req_type = POST_RP_INIT_REQUEST;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rps_post_rp_ready_init, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_clear_intus, SILIBS_SUCCESS);
    will_return_always(__wrap_oi_pcie_ss_set_laattr_rp_overrides, SILIBS_SUCCESS);
    expect_value(__wrap_enable_vab_isrs, vab_instances_to_init, (1 << r.rpss_index));
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

/* Test hide dpc is called if workaround is set */
TEST_FUNCTION(test_pcie_rpss_post_rp_ready_init_hide_dpc, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = POST_RP_INIT_REQUEST;
    r.req_type = POST_RP_INIT_REQUEST;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    pcie_prod_cfg_workarounds_t* rpss_workarounds = get_workaround_for_rpss(r.rpss_index);

    /* Set the workaround to true */
    rpss_workarounds->prod_rp_cfgs[0].hide_dpc = true;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rps_post_rp_ready_init, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_clear_intus, SILIBS_SUCCESS);
    will_return_always(__wrap_oi_pcie_ss_set_laattr_rp_overrides, SILIBS_SUCCESS);
    expect_value(__wrap_enable_vab_isrs, vab_instances_to_init, (1 << r.rpss_index));
    will_return(__wrap_oi_pcie_rp_dbi_hide_dpc_cap, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    /* Restore default value*/
    rpss_workarounds->prod_rp_cfgs[0].hide_dpc = false;
}

/* Test force read/write allocation workaround if flag is set*/
TEST_FUNCTION(test_pcie_rpss_post_rp_ready_init_force_allocation, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = POST_RP_INIT_REQUEST;
    r.req_type = POST_RP_INIT_REQUEST;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    pcie_prod_cfg_workarounds_t* rpss_workarounds = get_workaround_for_rpss(r.rpss_index);

    /* Set the workaround to true*/
    rpss_workarounds->prod_rp_cfgs[0].lattr_nosnoop_en_read = true;
    rpss_workarounds->prod_rp_cfgs[0].lattr_tph_en_read = true;
    rpss_workarounds->prod_rp_cfgs[0].lattr_notph_en_read = true;
    rpss_workarounds->prod_rp_cfgs[0].lattr_nosnoop_en_write = true;
    rpss_workarounds->prod_rp_cfgs[0].lattr_tph_en_write = true;
    rpss_workarounds->prod_rp_cfgs[0].lattr_notph_en_write = true;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rps_post_rp_ready_init, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_clear_intus, SILIBS_SUCCESS);
    expect_value(__wrap_enable_vab_isrs, vab_instances_to_init, (1 << r.rpss_index));
    will_return_always(__wrap_oi_pcie_ss_set_laattr_rp_overrides, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    /* Restore default value*/
    rpss_workarounds->prod_rp_cfgs[0].lattr_nosnoop_en_read = false;
    rpss_workarounds->prod_rp_cfgs[0].lattr_tph_en_read = false;
    rpss_workarounds->prod_rp_cfgs[0].lattr_notph_en_read = false;
    rpss_workarounds->prod_rp_cfgs[0].lattr_nosnoop_en_write = false;
    rpss_workarounds->prod_rp_cfgs[0].lattr_tph_en_write = false;
    rpss_workarounds->prod_rp_cfgs[0].lattr_notph_en_write = false;
}

TEST_FUNCTION(test_get_rp_ready_success, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = GET_RP_READY_REQUEST;
    r.req_type = GET_RP_READY_REQUEST;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rp_ready, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_get_rpss_ready_success, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = GET_RPSS_READY_REQUEST;
    r.req_type = GET_RPSS_READY_REQUEST;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rps_ready, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_get_rpss_ready_not_ready, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = GET_RPSS_READY_REQUEST;
    r.req_type = GET_RPSS_READY_REQUEST;
    r.rpss_index = RPSS1;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS1);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);

    /* Simulate RPSS not ready by returning a SILIBS_E_BUSY status */
    will_return(__wrap_pciess_rps_ready, SILIBS_E_BUSY);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_E_BUSY);
}

TEST_FUNCTION(test_begin_rp_post_link_up_init, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = POST_RP_LINK_UP_INIT;
    r.req_type = POST_RP_LINK_UP_INIT;
    r.rpss_index = RPSS3;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS3);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_pciess_rp_post_link_up_init, SILIBS_SUCCESS);
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, &mock_buf[0]);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_oi_pcie_ss_populate_rp_bdat, SILIBS_SUCCESS);
    expect_value(__wrap_atu_unmap, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    /* Ensure that on SVP it doesn't call into the silibs APIs */
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_get_rp_link_status, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = GET_LINK_STATUS;
    r.req_type = GET_LINK_STATUS;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rp_get_link_train_done, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_get_rp_link_status_logs_cper_busy, test_setup, test_teardown)
{
    pcie_sync_request_t r;
    memset(&r, 0, sizeof(r));
    r.header.RequestType = GET_LINK_STATUS;
    r.req_type = GET_LINK_STATUS;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    PDFWK_SYNC_REQUEST_HEADER req = (PDFWK_SYNC_REQUEST_HEADER)&r;
    req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

    mock_pcie_ent.id = RPSS2;
    mock_pcie_ent.rps[0].enabled = true;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rp_get_link_train_done, SILIBS_E_BUSY);
    will_return(__wrap_pcie_rp_sii_get_link_state, PCIE_LTSSM_STATE_DETECT_QUIET);
    expect_function_calls(__wrap_hm_submit_cper, 1);

    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_E_BUSY);
}

TEST_FUNCTION(test_get_rp_link_status_logs_cper_overwritten, test_setup, test_teardown)
{
    pcie_sync_request_t r;
    memset(&r, 0, sizeof(r));
    r.header.RequestType = GET_LINK_STATUS;
    r.req_type = GET_LINK_STATUS;
    r.rpss_index = RPSS3;
    r.rp_index = 1;

    PDFWK_SYNC_REQUEST_HEADER req = (PDFWK_SYNC_REQUEST_HEADER)&r;
    req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

    mock_pcie_ent.id = RPSS3;
    mock_pcie_ent.rps[1].enabled = true;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS3);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rp_get_link_train_done, SILIBS_E_OVERWRITTEN);
    will_return(__wrap_pcie_rp_sii_get_link_state, PCIE_LTSSM_STATE_RECOVERY_EQUALIZATION_0);
    expect_function_calls(__wrap_hm_submit_cper, 1);

    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_E_OVERWRITTEN);
}

TEST_FUNCTION(test_force_aer_internal_uncorr_err, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = FORCE_AER_INTERNAL_UNCORR_ERROR;
    r.req_type = FORCE_AER_INTERNAL_UNCORR_ERROR;
    r.rpss_index = RPSS2;
    r.rp_index = 0;
    r.p_requested_data = NULL;
    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pcie_ss_rp_aer_spoof, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pcie_ss_rp_aer_spoof, SILIBS_E_STATE);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_E_STATE);
}

/* IDE TX rekeying not supported yet - implement an actual test once done */
TEST_FUNCTION(test_ide_tx_rekey, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = IDE_TX_REKEY;
    r.req_type = IDE_TX_REKEY;
    r.rpss_index = RPSS2;
    r.rp_index = 0;
    r.p_requested_data = NULL;
    mock_pcie_ent.id = r.rpss_index;

    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

/* IDE RX rekeying not supported yet - implement an actual test once done */
TEST_FUNCTION(test_ide_rx_rekey, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = IDE_RX_REKEY;
    r.req_type = IDE_RX_REKEY;
    r.rpss_index = RPSS2;
    r.rp_index = 0;
    r.p_requested_data = NULL;
    mock_pcie_ent.id = r.rpss_index;

    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_error_injections, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    ras_einj_info_t einj_info;
    pcie_einj_operation_t* op = (pcie_einj_operation_t*)&(einj_info.status_operation);
    pcie_einj_params_t* pcie_params = (pcie_einj_params_t*)&(einj_info.param_type.error_parameters[0]);

    r.header.RequestType = INJECT_PCIE_ERROR;
    r.req_type = INJECT_PCIE_ERROR;
    r.rpss_index = RPSS2;
    r.rp_index = 0;
    r.p_requested_data = &einj_info;
    mock_pcie_ent.id = r.rpss_index;
    mock_pcie_ent.rps[0].bases._base = 0xdeadbeef;

    expect_any_always(__wrap_pciess_get_entity, rpss_idx);
    will_return_always(__wrap_pciess_get_entity, &mock_pcie_ent);

    pcie_params->sbdf.bus = 2;
    mock_pcie_ent.rps[0].valid = true;
    mock_pcie_ent.rps[0].bus_cfg.primary_bus = 0;
    mock_pcie_ent.rps[0].bus_cfg.subordinate_bus = 3;

    op->error_type = PCIE_ERROR_TYPE_AER;
    op->error_count = 1;
    pcie_params->error_data.aer = PCIE_SS_APP_ERR_BAD_TLP;
    will_return(__wrap_pcie_ss_rp_aer_spoof, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_CRC;
    op->error_count = 0;
    pcie_params->error_data.crc = PCIE_RASDES_INJ_DLLP_UPDATE_FC_CRC;
    will_return(__wrap_pcie_rp_rasdes_inject_crc_error, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_SEQNUM;
    op->error_count = 2;
    pcie_params->error_data.seqnum = PCIE_RASDES_INJ_TLP_SEQNUM;
    will_return(__wrap_pcie_rp_rasdes_inject_seqnum_error, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_DLLP;
    op->error_count = 2;
    pcie_params->error_data.dllp = PCIE_RASDES_INJ_DLLP_NACK_TIMEOUT;
    will_return(__wrap_pcie_rp_rasdes_inject_dllp_error, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_SYMBOL;
    op->error_count = 2;
    pcie_params->error_data.symbol = PCIE_RASDES_INJ_EIDL_SYMBOL;
    will_return(__wrap_pcie_rp_rasdes_inject_symbol_error, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_FC;
    op->error_count = 2;
    pcie_params->error_data.fc = PCIE_RASDES_INJ_POSTED_TLP_DATA_CREDIT;
    will_return(__wrap_pcie_rp_rasdes_inject_fc_error, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_RETRY_TLP;
    op->error_count = 2;
    pcie_params->error_data.retry_tlp = PCIE_RASDES_INJ_DUPLICATE_TLP;
    will_return(__wrap_pcie_rp_rasdes_inject_retry_tlp_error, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_PHY_SET_SRAM_ERROR;
    op->error_count = 1;
    pcie_params->error_data.phy_sram.phy_inst = 3;
    will_return(__wrap_pcie_phy_setup_error_injection, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_RP_INTERNAL_PHY_CLEAR_SRAM_ERROR;
    op->error_count = 1;
    pcie_params->error_data.phy_sram.phy_inst = 4;
    will_return(__wrap_pcie_phy_clear_injection, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_VSECRAS;
    op->error_count = 1;
    pcie_params->error_data.vsecras = RAS_PCIE_VSECRAS_L3_TX_UNCORRECTABLE_ERROR;
    will_return(__wrap_ras_pcie_vsecras_agent_trigger_by_type, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    op->error_type = PCIE_ERROR_TYPE_IDE;
    op->error_count = 1;
    pcie_params->error_data.ide = RAS_PCIE_IDE_DATAPATH_PROT_RX_DATA_UNCORRECTABLE_ERROR;
    will_return(__wrap_ras_pcie_ide_agent_trigger_by_type, SILIBS_SUCCESS);
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_error_injection_errors, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    ras_einj_info_t einj_info;
    pcie_einj_operation_t* op = (pcie_einj_operation_t*)&(einj_info.status_operation);
    pcie_einj_params_t* pcie_params = (pcie_einj_params_t*)&(einj_info.param_type.error_parameters[0]);

    r.header.RequestType = INJECT_PCIE_ERROR;
    r.req_type = INJECT_PCIE_ERROR;
    r.rpss_index = RPSS2;
    r.rp_index = 0;
    r.p_requested_data = &einj_info;
    mock_pcie_ent.id = r.rpss_index;
    mock_pcie_ent.rps[0].bases._base = 0xdeadbeef;

    expect_any_always(__wrap_pciess_get_entity, rpss_idx);
    will_return_always(__wrap_pciess_get_entity, &mock_pcie_ent);

    pcie_params->sbdf.bus = 4;
    mock_pcie_ent.rps[0].valid = true;
    mock_pcie_ent.rps[0].bus_cfg.primary_bus = 0;
    mock_pcie_ent.rps[0].bus_cfg.subordinate_bus = 3;

    op->error_type = PCIE_ERROR_TYPE_AER;
    op->error_count = 1;
    pcie_params->error_data.aer = PCIE_SS_APP_ERR_BAD_TLP;
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_E_PARAM);

    pcie_params->sbdf.bus = 2;
    mock_pcie_ent.rps[0].valid = true;
    mock_pcie_ent.rps[0].bus_cfg.primary_bus = 0;
    mock_pcie_ent.rps[0].bus_cfg.subordinate_bus = 3;

    op->error_type = PCIE_ERROR_TYPE_MAX;
    ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_E_PARAM);
}

TEST_FUNCTION(test_default_async_dispatch, test_setup, test_teardown)
{
    /* Setup the request */
    iface.dev = &dev;
    r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
    r.rpss_index = RPSS3;

    /* Check out of bounds root port index */
    r.rp_index = 50;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
    pcie_default_dispatch(&(r.header), nullptr);
    assert_int_equal(r.status, SILIBS_E_PARAM);
}

TEST_FUNCTION(test_dispatch_pool_full, test_setup, test_teardown)
{
    /* Setup the request */
    iface.dev = &dev;
    r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
    r.rpss_index = RPSS3;
    r.rp_index = 3;
    r.rp_op = WAIT_FOR_EVENT;

    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value_count(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT, -1);
    will_return_always(__wrap__txe_semaphore_get, TX_SUCCESS);
    expect_any_always(__wrap__txe_semaphore_put, semaphore_ptr);
    will_return_always(__wrap__txe_semaphore_put, TX_SUCCESS);

    pcie_per_rp_dispatch(&(r.header), nullptr);

    /*
     * If a request is queued again on the same RP it should be immediately completed as the
     * previous one is pending.
     */
    r.rpss_index = RPSS3;
    r.rp_index = 3;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
    pcie_per_rp_dispatch(&(r.header), nullptr);
}

TEST_FUNCTION(test_wait_for_event_dispatch, test_setup, test_teardown)
{
    /* Setup the request */
    iface.dev = &dev;
    r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
    r.rpss_index = RPSS3;
    r.rp_index = 3;
    r.rp_op = WAIT_FOR_EVENT;

    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value_count(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT, -1);
    will_return_always(__wrap__txe_semaphore_get, TX_SUCCESS);
    expect_any_always(__wrap__txe_semaphore_put, semaphore_ptr);
    will_return_always(__wrap__txe_semaphore_put, TX_SUCCESS);
    pcie_per_rp_dispatch(&(r.header), nullptr);
}

TEST_FUNCTION(test_unknown_req_dispatch, test_setup, test_teardown)
{
    /* Setup the request */
    iface.dev = &dev;
    r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
    r.rpss_index = RPSS3;
    r.rp_index = 3;
    r.rp_op = PCIE_MAX_ASYNC_REQ;

    pcie_per_rp_dispatch(&(r.header), nullptr);
    assert_int_equal(r.status, SILIBS_E_PARAM);
}

TEST_FUNCTION(test_get_async_req_invalid_error_paths, test_setup, test_teardown)
{
    pcie_async_request_t* req;
    req = get_pending_async_req_for_this_rp(0xFF, 0xFF, WAIT_FOR_EVENT);
    assert_ptr_equal(req, nullptr);

    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value_count(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT, -1);
    will_return(__wrap__txe_semaphore_get, TX_SEMAPHORE_ERROR);
    req = get_pending_async_req_for_this_rp(RPSS0, 0, WAIT_FOR_EVENT);
    assert_ptr_equal(req, nullptr);
}

TEST_FUNCTION(test_complete_async_req_error_paths, test_setup, test_teardown)
{
    r.rpss_index = RPSS0;
    r.rp_index = 0;
    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT);
    will_return(__wrap__txe_semaphore_get, TX_SEMAPHORE_ERROR);
    /* Ensure no completions are sent if we cannot get the semaphore */
    complete_async_req_for_this_rp(&r);

    /* Out of range indices are passed for rpss/rp*/
    r.rpss_index = (RPSS_INSTANCE)(NUM_RPSS);
    r.rp_index = 0xFF;
    complete_async_req_for_this_rp(&r);

    /* Ensure the function returns without doing anything incase a null input is passed */
    complete_async_req_for_this_rp(nullptr);
}

TEST_FUNCTION(test_rpss_int_link_up, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value_count(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT, -1);
    will_return_always(__wrap__txe_semaphore_get, TX_SUCCESS);
    expect_any_always(__wrap__txe_semaphore_put, semaphore_ptr);
    will_return_always(__wrap__txe_semaphore_put, TX_SUCCESS);

    for (uint8_t i = RPSS0; i < RPSS4; i++)
    {
        /* Setup the request */
        memset(&r, 0x0, sizeof(r));
        memset(&mock_int_info, 0x0, sizeof(mock_int_info));
        iface.dev = &dev;
        r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
        r.rpss_index = (RPSS_INSTANCE)i;
        r.rp_index = 0;
        r.rp_op = WAIT_FOR_EVENT;

        add_async_req_to_pool(&r);
        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.ss_type = PCIE_SS_X16;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = false;
        mock_pcie_ent.rps[2].enabled = false;
        mock_pcie_ent.rps[3].enabled = false;

        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_LINK_UP].asserted = true;
        mock_int_info.rp_ints[1].ints[PCIESS_RP_INT_LINK_UP].asserted = false;
        mock_int_info.rp_ints[2].ints[PCIESS_RP_INT_LINK_UP].asserted = false;
        mock_int_info.rp_ints[3].ints[PCIESS_RP_INT_LINK_UP].asserted = false;

        expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
        rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        assert_int_equal(r.async_data.int_mask, (1 << PCIESS_RP_INT_LINK_UP));
    }
}

TEST_FUNCTION(test_rpss_int_link_down, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value_count(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT, -1);
    will_return_always(__wrap__txe_semaphore_get, TX_SUCCESS);
    expect_any_always(__wrap__txe_semaphore_put, semaphore_ptr);
    will_return_always(__wrap__txe_semaphore_put, TX_SUCCESS);

    for (uint8_t i = RPSS0; i < RPSS4; i++)
    {
        /* Setup the request */
        memset(&r, 0x0, sizeof(r));
        memset(&mock_int_info, 0x0, sizeof(mock_int_info));
        iface.dev = &dev;
        r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
        r.rpss_index = (RPSS_INSTANCE)i;
        r.rp_index = 0;
        r.rp_op = WAIT_FOR_EVENT;

        add_async_req_to_pool(&r);
        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.ss_type = PCIE_SS_X16;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = false;
        mock_pcie_ent.rps[2].enabled = false;
        mock_pcie_ent.rps[3].enabled = false;

        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_LINK_DOWN].asserted = true;
        mock_int_info.rp_ints[1].ints[PCIESS_RP_INT_LINK_DOWN].asserted = false;
        mock_int_info.rp_ints[2].ints[PCIESS_RP_INT_LINK_DOWN].asserted = false;
        mock_int_info.rp_ints[3].ints[PCIESS_RP_INT_LINK_DOWN].asserted = false;

        /* Setup silibs expectations */
        expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
        rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        assert_int_equal(r.async_data.int_mask, (1 << PCIESS_RP_INT_LINK_DOWN));
    }
}

TEST_FUNCTION(test_rpss_int_global_ide_fatal_errors, test_setup, test_teardown)
{
    for (uint8_t i = RPSS0; i < RPSS4; i++)
    {
        /* Setup mocks */
        memset(&mock_int_info, 0x0, sizeof(mock_int_info));

        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.ss_type = PCIE_SS_X16;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = false;
        mock_pcie_ent.rps[2].enabled = false;
        mock_pcie_ent.rps[3].enabled = false;

        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_GLOBAL_IDE].asserted = true;
        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_GLOBAL_IDE].status =
            (PCIE_IDE_SRAM_ECC_GLOBAL_INT | PCIE_IDE_DATAPATH_GLOBAL_INT | PCIE_IDE_MISC_GLOBAL_INT);
        mock_int_info.rp_ints[1].ints[PCIESS_RP_INT_GLOBAL_IDE].asserted = false;
        mock_int_info.rp_ints[2].ints[PCIESS_RP_INT_GLOBAL_IDE].asserted = false;
        mock_int_info.rp_ints[3].ints[PCIESS_RP_INT_GLOBAL_IDE].asserted = false;

        /* Setup silibs expectations */
        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_KEY_SRAM_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, true);
        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_KEY_SRAM_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, false);
        will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
        will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
        expect_function_calls(__wrap_hm_submit_cper, 1);
        will_return(__wrap_pciess_ras_process_record, SILIBS_SUCCESS);

        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_DATAPATH_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, true);
        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_DATAPATH_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, false);
        will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
        will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
        expect_function_calls(__wrap_hm_submit_cper, 1);
        will_return(__wrap_pciess_ras_process_record, SILIBS_SUCCESS);

        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_MISC_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, true);
        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_MISC_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, false);
        will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
        will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
        expect_function_calls(__wrap_hm_submit_cper, 1);
        will_return(__wrap_pciess_ras_process_record, SILIBS_SUCCESS);

        expect_function_calls(__wrap_crash_dump_bug_check, 1);
        if (!bugcheck_mock_return())
        {
            rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        }
    }
}

TEST_FUNCTION(test_rpss_int_global_ide_non_fatal_errors, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value_count(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT, -1);
    will_return_always(__wrap__txe_semaphore_get, TX_SUCCESS);
    expect_any_always(__wrap__txe_semaphore_put, semaphore_ptr);
    will_return_always(__wrap__txe_semaphore_put, TX_SUCCESS);

    for (uint8_t i = RPSS0; i < RPSS4; i++)
    {
        /* Setup the request */
        memset(&r, 0x0, sizeof(r));
        memset(&mock_int_info, 0x0, sizeof(mock_int_info));
        iface.dev = &dev;
        r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
        r.rpss_index = (RPSS_INSTANCE)i;
        r.rp_index = 0;
        r.rp_op = WAIT_FOR_EVENT;
        add_async_req_to_pool(&r);

        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.ss_type = PCIE_SS_X16;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = false;
        mock_pcie_ent.rps[2].enabled = false;
        mock_pcie_ent.rps[3].enabled = false;

        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_GLOBAL_IDE].asserted = true;
        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_GLOBAL_IDE].status =
            (PCIE_IDE_KEY_NEEDED_INT | PCIE_IDE_MISC_GLOBAL_INT);
        mock_int_info.rp_ints[1].ints[PCIESS_RP_INT_GLOBAL_IDE].asserted = false;
        mock_int_info.rp_ints[2].ints[PCIESS_RP_INT_GLOBAL_IDE].asserted = false;
        mock_int_info.rp_ints[3].ints[PCIESS_RP_INT_GLOBAL_IDE].asserted = false;

        /* Setup silibs expectations */
        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_MISC_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, true);
        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_MISC_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, false);
        will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL);
        will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
        expect_function_calls(__wrap_hm_submit_cper, 1);
        will_return(__wrap_pciess_ras_process_record, SILIBS_SUCCESS);
        will_return(__wrap_pcie_rp_ide_disable_all_streams, SILIBS_SUCCESS);
        expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
        rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        assert_int_equal(r.async_data.int_mask, (1 << PCIESS_RP_INT_GLOBAL_IDE));
        assert_int_equal(r.async_data.glbl_ide_data, (PCIE_IDE_KEY_NEEDED_INT | PCIE_IDE_MISC_GLOBAL_INT));
    }
}

TEST_FUNCTION(test_rpss_int_aes_hcfg, test_setup, test_teardown)
{
    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value_count(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT, -1);
    will_return_always(__wrap__txe_semaphore_get, TX_SUCCESS);
    expect_any_always(__wrap__txe_semaphore_put, semaphore_ptr);
    will_return_always(__wrap__txe_semaphore_put, TX_SUCCESS);

    for (uint8_t i = RPSS0; i < RPSS4; i++)
    {
        /* Setup the request */
        memset(&r, 0x0, sizeof(r));
        memset(&mock_int_info, 0x0, sizeof(mock_int_info));
        iface.dev = &dev;
        r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
        r.rpss_index = (RPSS_INSTANCE)i;
        r.rp_index = 0;
        r.rp_op = WAIT_FOR_EVENT;

        add_async_req_to_pool(&r);

        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = false;
        mock_pcie_ent.rps[2].enabled = false;
        mock_pcie_ent.rps[3].enabled = false;

        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_AES_HCFG].asserted = true;
        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_AES_HCFG].status = PCIE_IDE_TX_KEY_DONE_INT;
        mock_int_info.rp_ints[1].ints[PCIESS_RP_INT_AES_HCFG].asserted = false;
        mock_int_info.rp_ints[2].ints[PCIESS_RP_INT_AES_HCFG].asserted = false;
        mock_int_info.rp_ints[3].ints[PCIESS_RP_INT_AES_HCFG].asserted = false;

        /* Setup silibs expectations */
        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_AES_HCFG_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, true);
        expect_value(__wrap_ras_pcie_ide_agent_probe_by_type, types, RAS_PCIE_IDE_ALL_AES_HCFG_ERRORS);
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ide_agent_probe_by_type, false);
        will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL);
        will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
        expect_function_calls(__wrap_hm_submit_cper, 1);
        will_return(__wrap_pciess_ras_process_record, SILIBS_SUCCESS);
        expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
        rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        assert_int_equal(r.async_data.int_mask, (1 << PCIESS_RP_INT_AES_HCFG));
        assert_int_equal(r.async_data.aes_hcfg_data, PCIE_IDE_TX_KEY_DONE_INT);
    }
}

TEST_FUNCTION(test_rpss_int_dtim, test_setup, test_teardown)
{
    for (uint8_t i = RPSS0; i < RPSS4; i++)
    {
        /* Setup the request */
        memset(&mock_int_info, 0x0, sizeof(mock_int_info));

        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = false;
        mock_pcie_ent.rps[2].enabled = false;
        mock_pcie_ent.rps[3].enabled = false;

        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_DTIM].asserted = true;
        mock_int_info.rp_ints[1].ints[PCIESS_RP_INT_DTIM].asserted = false;
        mock_int_info.rp_ints[2].ints[PCIESS_RP_INT_DTIM].asserted = false;
        mock_int_info.rp_ints[3].ints[PCIESS_RP_INT_DTIM].asserted = false;

        /* Setup silibs expectations */
        expect_value(__wrap_ras_pcie_dtim_agent_probe_by_type, types, RAS_PCIE_DTIM_ALL_TYPES);
        will_return(__wrap_ras_pcie_dtim_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_dtim_agent_probe_by_type, true);
        expect_value(__wrap_ras_pcie_dtim_agent_probe_by_type, types, RAS_PCIE_DTIM_ALL_TYPES);
        will_return(__wrap_ras_pcie_dtim_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_dtim_agent_probe_by_type, false);
        will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
        will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
        expect_function_calls(__wrap_hm_submit_cper, 1);
        will_return(__wrap_pciess_ras_process_record, SILIBS_SUCCESS);
        expect_function_calls(__wrap_crash_dump_bug_check, 1);
        if (!bugcheck_mock_return())
        {
            rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        }
    }
}

TEST_FUNCTION(test_rpss_int_ltim, test_setup, test_teardown)
{
    for (uint8_t i = RPSS0; i < RPSS4; i++)
    {
        /* Setup the request */
        memset(&mock_int_info, 0x0, sizeof(mock_int_info));

        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = false;
        mock_pcie_ent.rps[2].enabled = false;
        mock_pcie_ent.rps[3].enabled = false;

        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_LTIM].asserted = true;
        mock_int_info.rp_ints[1].ints[PCIESS_RP_INT_LTIM].asserted = false;
        mock_int_info.rp_ints[2].ints[PCIESS_RP_INT_LTIM].asserted = false;
        mock_int_info.rp_ints[3].ints[PCIESS_RP_INT_LTIM].asserted = false;

        /* Setup silibs expectations */
        expect_value(__wrap_ras_pcie_ltim_agent_probe_by_type, types, RAS_PCIE_LTIM_ALL_TYPES);
        will_return(__wrap_ras_pcie_ltim_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ltim_agent_probe_by_type, true);
        expect_value(__wrap_ras_pcie_ltim_agent_probe_by_type, types, RAS_PCIE_LTIM_ALL_TYPES);
        will_return(__wrap_ras_pcie_ltim_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_ltim_agent_probe_by_type, false);
        will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
        will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
        expect_function_calls(__wrap_hm_submit_cper, 1);
        will_return(__wrap_pciess_ras_process_record, SILIBS_SUCCESS);
        expect_function_calls(__wrap_crash_dump_bug_check, 1);
        if (!bugcheck_mock_return())
        {
            rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        }
    }
}

TEST_FUNCTION(test_rpss_int_rasdp, test_setup, test_teardown)
{
    for (uint8_t i = RPSS0; i < RPSS4; i++)
    {
        /* Setup the request */
        memset(&mock_int_info, 0x0, sizeof(mock_int_info));

        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = false;
        mock_pcie_ent.rps[2].enabled = false;
        mock_pcie_ent.rps[3].enabled = false;

        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_RASDP].asserted = true;
        mock_int_info.rp_ints[1].ints[PCIESS_RP_INT_RASDP].asserted = false;
        mock_int_info.rp_ints[2].ints[PCIESS_RP_INT_RASDP].asserted = false;
        mock_int_info.rp_ints[3].ints[PCIESS_RP_INT_RASDP].asserted = false;

        /* Setup silibs expectations */
        expect_value(__wrap_ras_pcie_vsecras_agent_probe_by_type, types, RAS_UNCORRECTABLE_ERROR);
        will_return(__wrap_ras_pcie_vsecras_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_vsecras_agent_probe_by_type, true);
        will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTABLE_FATAL);
        will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
        expect_function_calls(__wrap_hm_submit_cper, 1);
        will_return(__wrap_pciess_ras_process_record, SILIBS_SUCCESS);
        expect_value(__wrap_ras_pcie_vsecras_agent_probe_by_type, types, RAS_CORRECTABLE_ERROR);
        will_return(__wrap_ras_pcie_vsecras_agent_probe_by_type, &(mock_pcie_ent.rps[0]));
        will_return(__wrap_ras_pcie_vsecras_agent_probe_by_type, true);
        will_return(__wrap_ras_agent_record_to_cper, ACPI_ERROR_SEVERITY_UNCORRECTED_NON_FATAL);
        will_return(__wrap_ras_agent_record_to_cper, SILIBS_SUCCESS);
        expect_function_calls(__wrap_hm_submit_cper, 1);
        will_return(__wrap_pciess_ras_process_record, SILIBS_SUCCESS);
        will_return(__wrap_pcie_rp_vsecras_clear_rasdp_error_mode, SILIBS_SUCCESS);
        expect_function_calls(__wrap_crash_dump_bug_check, 1);

        if (!bugcheck_mock_return())
        {
            rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        }
    }
}

TEST_FUNCTION(test_rpss_int_non_por, test_setup, test_teardown)
{
    for (uint8_t non_por_idx = PCIESS_RP_INT_HP_PME; non_por_idx <= PCIESS_RP_INT_PM_TO_ACK; non_por_idx++)
    {
        for (uint8_t i = RPSS0; i < RPSS4; i++)
        {
            memset(&mock_int_info, 0x0, sizeof(mock_int_info));

            mock_pcie_ent.id = (RPSS_INSTANCE)i;
            mock_pcie_ent.rps[0].enabled = true;
            mock_pcie_ent.rps[1].enabled = false;
            mock_pcie_ent.rps[2].enabled = false;
            mock_pcie_ent.rps[3].enabled = false;

            mock_int_info.rp_ints[0].ints[non_por_idx].asserted = true;
            mock_int_info.rp_ints[0].ints[non_por_idx].status = 0x0;
            rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        }
    }

    for (uint8_t non_por_idx = PCIESS_RP_INT_SEND_C; non_por_idx <= PCIESS_RP_INT_SEND_F; non_por_idx++)
    {
        for (uint8_t i = RPSS0; i < RPSS4; i++)
        {
            memset(&mock_int_info, 0x0, sizeof(mock_int_info));

            mock_pcie_ent.id = (RPSS_INSTANCE)i;
            mock_pcie_ent.rps[0].enabled = true;
            mock_pcie_ent.rps[1].enabled = false;
            mock_pcie_ent.rps[2].enabled = false;
            mock_pcie_ent.rps[3].enabled = false;

            mock_int_info.rp_ints[0].ints[non_por_idx].asserted = true;
            mock_int_info.rp_ints[0].ints[non_por_idx].status = 0x0;
            rpss_irq_callback(&mock_pcie_ent, &mock_int_info);
        }
    }
}

TEST_FUNCTION(test_rpss_init_cxl, test_setup, test_teardown)
{
    /*Setup the mock interface and device*/
    pcie_root_bridge_config rb_configs[4];
    pciess_device_t dev;
    dev.rb_configs = rb_configs;
    pciess_device_interface_t iface;
    iface.dev = &dev;

    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = INITIAL_CONFIG_REQUEST;
    r.req_type = INITIAL_CONFIG_REQUEST;
    r.rpss_index = RPSS5; // RPSS5 on FPGA is CXL-enabled
    r.rp_index = 0;

    /* Set the owning interface*/
    PDFWK_SYNC_REQUEST_HEADER req = (PDFWK_SYNC_REQUEST_HEADER)&r;
    req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

    mock_pcie_ent.id = r.rpss_index;

    /* Setup silibs expectations */
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA);
    will_return(__wrap_get_rpss_resolved_base, 0xDEADBEEF);
    will_return_always(__wrap_system_info_get_soc_position, 0x1);
    bool is_mirroring = __real_config_get_pcie_configuration_mirroring();
    is_mirroring = true;
    will_return(__wrap_config_get_pcie_configuration_mirroring, is_mirroring);
    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS5);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    expect_value(__wrap_pciess_config_entity, program_phy_regs, false);
    expect_value(__wrap_pciess_config_entity, enable_apu, true);
    will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
    will_return(__wrap_pciess_config_ss_for_bifur, SILIBS_SUCCESS);
    will_return(__wrap_pciess_deassert_por_reset, SILIBS_SUCCESS);
    will_return(__wrap_pciess_phys_toggle_clocks, SILIBS_SUCCESS);
    will_return(__wrap_idsw_get_die_id, DIE_1);

    cxl_region_params_t cxl_region_params_die1 = __real_config_get_cxl_params_die1();
    cxl_region_params_die1.valid = true;
    cxl_region_params_die1.interleave_ways = INTERLEAVE_NONE;
    cxl_region_params_die1.ports[0] = CXL_RPSS5_RP0;
    will_return(__wrap_config_get_cxl_params_die1, &cxl_region_params_die1);

    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);

    assert_int_equal(rb_configs[0].flags.is_enabled, true);
    assert_int_equal(rb_configs[0].flags.is_cxl, true);
    assert_int_equal(rb_configs[0].flags.is_secondary_soc, 0b1);
}

/* Test case for initial pciess init sync. request for non-mirrored configurations successful path with IFT enabled */
TEST_FUNCTION(test_pcie_rpss_init_soc1_ift_success, test_setup, test_teardown)
{
    // Setup Enabled IFT
    ift_enabled = true;

    /*Setup the mock interface and device*/
    pcie_root_bridge_config rb_configs[4];
    pciess_device_t dev;
    dev.rb_configs = rb_configs;
    pciess_device_interface_t iface;
    iface.dev = &dev;

    /* Setup silibs expectations */
    /* Will always is used because IS_PLATFORM_FPGA() uses 3 calls to idsw_get_platform_sdv */
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    for (uint8_t i = 0; i < NUM_RPSS; i++)
    {
        /* Setup the request for an rpss */
        pcie_sync_request_t r;
        r.header.RequestType = INITIAL_CONFIG_REQUEST;
        r.req_type = INITIAL_CONFIG_REQUEST;
        r.rpss_index = (RPSS_INSTANCE)i;
        r.rp_index = 0;

        /* Set the owning interface*/
        auto req = (PDFWK_SYNC_REQUEST_HEADER)&r;
        req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

        mock_pcie_ent.id = r.rpss_index;
        will_return(__wrap_get_rpss_resolved_base, 0xDEADBEEF);
        expect_value(__wrap_pciess_get_entity, rpss_idx, i);
        will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
        will_return(__wrap_system_info_get_soc_position, 0x00);
        expect_value(__wrap_pciess_config_entity, program_phy_regs, true);
        expect_value(__wrap_pciess_config_entity, enable_apu, true);
        will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
        will_return(__wrap_pciess_config_ss_for_ift, SILIBS_SUCCESS);
        will_return(__wrap_idsw_get_die_id, DIE_0);

        cxl_region_params_t cxl_region_params_die0 = __real_config_get_cxl_params_die0();
        will_return(__wrap_config_get_cxl_params_die0, &cxl_region_params_die0);

        int32_t ret = pcie_sched_sync_op(&(r.header));
        assert_int_equal(ret, 0);
        assert_int_equal(r.status, SILIBS_SUCCESS);
    }

    // Cleanup IFT disabled
    ift_enabled = false;
}

/* Test case for initial pciess init sync. request for non-mirrored configurations successful path with IFT enabled */
TEST_FUNCTION(test_pcie_rpss_init_soc1_ift_fail, test_setup, test_teardown)
{
    // Setup Enabled IFT
    ift_enabled = true;

    /*Setup the mock interface and device*/
    pcie_root_bridge_config rb_configs[4];
    pciess_device_t dev;
    dev.rb_configs = rb_configs;
    pciess_device_interface_t iface;
    iface.dev = &dev;

    /* Setup silibs expectations */
    /* Will always is used because IS_PLATFORM_FPGA() uses 3 calls to idsw_get_platform_sdv */
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    expect_function_calls(__wrap_crash_dump_bug_check, 1);

    if (!bugcheck_mock_return())
    {
        /* Setup the request for an rpss */
        pcie_sync_request_t r;
        r.header.RequestType = INITIAL_CONFIG_REQUEST;
        r.req_type = INITIAL_CONFIG_REQUEST;
        r.rpss_index = RPSS0;
        r.rp_index = 0;

        /* Set the owning interface*/
        auto req = (PDFWK_SYNC_REQUEST_HEADER)&r;
        req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

        mock_pcie_ent.id = r.rpss_index;
        will_return(__wrap_get_rpss_resolved_base, 0xDEADBEEF);
        expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS0);
        will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
        will_return(__wrap_system_info_get_soc_position, 0x00);
        expect_value(__wrap_pciess_config_entity, program_phy_regs, true);
        expect_value(__wrap_pciess_config_entity, enable_apu, true);
        will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
        will_return(__wrap_pciess_config_ss_for_ift, SILIBS_E_ACCESS);
        will_return(__wrap_idsw_get_die_id, DIE_0);

        cxl_region_params_t cxl_region_params_die0 = __real_config_get_cxl_params_die0();
        will_return(__wrap_config_get_cxl_params_die0, &cxl_region_params_die0);

        pcie_sched_sync_op(&(r.header));
    }

    // Cleanup IFT disabled
    ift_enabled = false;
}

TEST_FUNCTION(test_get_config, test_setup, test_teardown)
{
    pcie_cfg_t* cfg;

    for (uint8_t i = RPSS0; i < NUM_RPSS; i++)
    {
        cfg = nullptr;
        cfg = get_configuration_for_rpss(i);
        assert_non_null(cfg);
    }

    /* Test that get_configuration_for_rpss rejects invalid RPSS ids */
    expect_function_calls(__wrap_crash_dump_bug_check, 1);
    if (!bugcheck_mock_return())
    {
        get_configuration_for_rpss(NUM_RPSS);
    }

    expect_function_calls(__wrap_crash_dump_bug_check, 1);
    if (!bugcheck_mock_return())
    {
        get_configuration_for_rpss((NUM_RPSS + 5));
    }
}

TEST_FUNCTION(test_get_workarounds, test_setup, test_teardown)
{
    pcie_prod_cfg_workarounds_t* pcie_workaround_cfg;
    for (uint8_t i = RPSS0; i < NUM_RPSS; i++)
    {
        pcie_workaround_cfg = nullptr;
        pcie_workaround_cfg = get_workaround_for_rpss(i);
        assert_non_null(pcie_workaround_cfg);
    }
    /* Test that get_workaround_for_rpss rejects invalid RPSS ids */
    expect_function_calls(__wrap_crash_dump_bug_check, 1);
    if (!bugcheck_mock_return())
    {
        get_workaround_for_rpss(NUM_RPSS);
    }
}

TEST_FUNCTION(test_get_pcie_bdat_info_errors, test_setup, test_teardown)
{
    /* Make atu map fail */
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, &mock_buf[0]);
    will_return(__wrap_atu_map, SILIBS_E_RANGE);

    mock_pcie_ent.id = (RPSS_INSTANCE)0;
    mock_pcie_ent.rps[0].enabled = true;
    mock_pcie_ent.rps[1].enabled = false;
    mock_pcie_ent.rps[2].enabled = false;
    mock_pcie_ent.rps[3].enabled = false;

    silibs_status_t status = publish_pcie_bdat_info_for_this_rp(&mock_pcie_ent, 0);
    assert_int_equal(status, SILIBS_E_RANGE);

    /*
     * Now ensure that we aren't attempting to repopulate bdat data for rpss 0
     * (even though the earlier init failed)
     */
    status = publish_pcie_bdat_info_for_this_rp(&mock_pcie_ent, 0);
    assert_int_equal(status, SILIBS_SUCCESS);
}
