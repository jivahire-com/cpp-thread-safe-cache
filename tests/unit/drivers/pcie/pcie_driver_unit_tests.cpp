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
#include <DfwkPtrTypes.h>
#include <error_handler.h>
#include <kng_soc_constants.h>
#include <pcie_config_i.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <pcie_irq.h>
#include <pcie_ss_common.h> // for pcie_ss_entity_t
#include <silibs_status.h>  // for SILIBS_E_PARAM, SILIBS_SUCCESS
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
#define MAX_ASYNC_REQ_POOL_SIZE (16)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pciess_device_t dev;
static pciess_device_interface_t iface;
static pcie_async_request_t r;
static pciess_int_probe_t mock_int_info;
/* mock entity*/
pcie_ss_entity_t mock_pcie_ent;

/*------------- Functions ----------------*/
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
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
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
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
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
        expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
        will_return(__wrap_atu_map, SILIBS_SUCCESS);
        expect_value(__wrap_pciess_get_entity, rpss_idx, i);
        will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
        will_return(__wrap_system_info_get_board_id, 0x00);
        expect_value(__wrap_pciess_config_entity, program_phy_regs, true);
        expect_value(__wrap_pciess_config_entity, enable_apu, true);
        will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
        will_return(__wrap_pciess_config_ss_for_bifur, SILIBS_SUCCESS);
        will_return(__wrap_pciess_deassert_por_reset, SILIBS_SUCCESS);
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
        expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
        will_return(__wrap_atu_map, SILIBS_SUCCESS);
        expect_value(__wrap_pciess_get_entity, rpss_idx, i);
        will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
        will_return(__wrap_system_info_get_board_id, 0xFF);
        bool is_mirroring = __real_config_get_pcie_configuration_mirroring();
        is_mirroring = true;
        will_return(__wrap_config_get_pcie_configuration_mirroring, is_mirroring);
        expect_value(__wrap_pciess_config_entity, program_phy_regs, true);
        expect_value(__wrap_pciess_config_entity, enable_apu, true);
        will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
        will_return(__wrap_pciess_config_ss_for_bifur, SILIBS_SUCCESS);
        will_return(__wrap_pciess_deassert_por_reset, SILIBS_SUCCESS);
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
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return_always(__wrap_system_info_get_board_id, 0x00);
    expect_value(__wrap_pciess_config_entity, program_phy_regs, false);
    expect_value(__wrap_pciess_config_entity, enable_apu, true);
    will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
    will_return(__wrap_pciess_config_ss_for_bifur, SILIBS_SUCCESS);
    will_return(__wrap_pciess_deassert_por_reset, SILIBS_SUCCESS);
    will_return(__wrap_idsw_get_die_id, DIE_0);

    cxl_region_params_t cxl_region_params_die0 = __real_config_get_cxl_params_die0();
    will_return(__wrap_config_get_cxl_params_die0, &cxl_region_params_die0);

    pcie_sched_sync_op(&(r.header));

    assert_int_equal(rb_configs[0].flags.is_enabled, true);
    assert_int_equal(rb_configs[3].flags.is_enabled, true);
    assert_int_equal(rb_configs[0].flags.is_secondary_soc, false);
}

/* Test case for initial pciess init sync. request atu mapping failures */
TEST_FUNCTION(test_pcie_rpss_init_atu_map_fail, test_setup, test_teardown)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = INITIAL_CONFIG_REQUEST;
    r.req_type = INITIAL_CONFIG_REQUEST;
    r.rpss_index = RPSS2;
    r.rp_index = 0;

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_E_RANGE);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        pcie_sched_sync_op(&(r.header));
    }
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
    will_return(__wrap_pciess_phys_sram_init_done, SILIBS_SUCCESS);
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
    will_return(__wrap_pciess_rps_ready, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_post_rp_ready_init, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_clear_intus, SILIBS_SUCCESS);
    expect_value(__wrap_enable_vab_isrs, vab_instances_to_init, (1 << r.rpss_index));
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

/* Test hide dpc is called if workaround is set*/
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

    /* Set the workaround to true*/
    rpss_workarounds->prod_rp_cfgs[0].hide_dpc = true;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rps_ready, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_post_rp_ready_init, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_clear_intus, SILIBS_SUCCESS);
    expect_value(__wrap_enable_vab_isrs, vab_instances_to_init, (1 << r.rpss_index));
    will_return(__wrap_oi_pcie_rp_dbi_hide_dpc_cap, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);

    /* Restore default value*/
    rpss_workarounds->prod_rp_cfgs[0].hide_dpc = false;
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

TEST_FUNCTION(test_invalid_irq, test_setup, test_teardown)
{
    will_return_always(__wrap_idsw_get_die_id, 0);
    uint32_t irq_num = HW_INT_VAB4_COMBINED_SCP_INT;
    for (uint8_t i = RPSS0; i < RPSS1; i++, irq_num++)
    {
        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = true;
        mock_pcie_ent.rps[2].enabled = true;
        mock_pcie_ent.rps[3].enabled = true;

        expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
        if (!set_error_handler_return())
        {
            rpss_irq_callback(irq_num);
        }
    }
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

TEST_FUNCTION(test_rpss_int, test_setup, test_teardown)
{
    will_return_always(__wrap_idsw_get_die_id, 0);
    expect_any_always(__wrap__txe_semaphore_get, semaphore_ptr);
    expect_value_count(__wrap__txe_semaphore_get, wait_option, TX_NO_WAIT, -1);
    will_return_always(__wrap__txe_semaphore_get, TX_SUCCESS);
    expect_any_always(__wrap__txe_semaphore_put, semaphore_ptr);
    will_return_always(__wrap__txe_semaphore_put, TX_SUCCESS);

    uint32_t irq_num = HW_INT_VAB0_COMBINED_SCP_INT;
    for (uint8_t i = RPSS0; i < RPSS4; i++, irq_num++)
    {
        /* Setup the request */
        iface.dev = &dev;
        r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
        r.rpss_index = (RPSS_INSTANCE)i;
        ;
        r.rp_index = 0;
        r.rp_op = WAIT_FOR_EVENT;

        add_async_req_to_pool(&r);
        mock_pcie_ent.id = (RPSS_INSTANCE)i;
        mock_pcie_ent.rps[0].enabled = true;
        mock_pcie_ent.rps[1].enabled = false;
        mock_pcie_ent.rps[2].enabled = false;
        mock_pcie_ent.rps[3].enabled = false;

        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_LINK_DOWN].asserted = true;
        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_LINK_UP].asserted = true;
        mock_int_info.rp_ints[0].ints[PCIESS_RP_INT_DPC].asserted = true;

        /* Setup silibs expectations */
        expect_value(__wrap_pciess_get_entity, rpss_idx, i);
        will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
        expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
        will_return(__wrap_pciess_probe, true);

        rpss_irq_callback(irq_num);
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
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return_always(__wrap_system_info_get_board_id, 0xFF);
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
    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
    {
        get_configuration_for_rpss(NUM_RPSS);
    }

    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
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
    expect_value(FPFwErrorRaise, error, (uint32_t)-1);
    if (!set_error_handler_return())
    {
        get_workaround_for_rpss(NUM_RPSS);
    }
}
