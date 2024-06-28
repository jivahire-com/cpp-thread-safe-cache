//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * This section is used to describe the file and any relevant
 * information related to it purpose and how it should be used.
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <atu_lib.h>
#include <cmocka.h> // IWYU pragma: keep
#include <cstddef>  // IWYU pragma: keep
#include <cstdint>  // IWYU pragma: keep
#include <idsw_kng.h>
#include <pcie_common.h>
#include <pcie_ss_common.h>

extern "C" {
#include <DfwkPtrTypes.h>
#include <error_handler.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <pcie_ss_common.h> // for pcie_ss_entity_t
#include <silibs_status.h>  // for SILIBS_E_PARAM, SILIBS_SUCCESS
#include <tx_api.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
pciess_device_t dev;
pciess_device_interface_t iface;

/* mock entity*/
pcie_ss_entity_t mock_pcie_ent;

/*------------- Functions ----------------*/
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

    /* No need to mock any dfwk functionality in the good case as of now */
    pcie_dfwk_init(&dev, sched);
}

/* Test cases for top-level interface initialization */
TEST_FUNCTION(interface_init, NULL, NULL)
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

/* Test case for initial pciess init sync. request successful path */
TEST_FUNCTION(test_pcie_rpss_init_success, NULL, NULL)
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

    /* Set the owning interface*/
    PDFWK_SYNC_REQUEST_HEADER req = (PDFWK_SYNC_REQUEST_HEADER)&r;
    req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

    mock_pcie_ent.id = r.rpss_index;

    /* Setup silibs expectations */
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    expect_value(__wrap_pciess_config_entity, program_phy_regs, true);
    expect_value(__wrap_pciess_config_entity, enable_apu, true);
    will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
    will_return(__wrap_pciess_config_ss_for_bifur, SILIBS_SUCCESS);
    will_return(__wrap_pciess_deassert_por_reset, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

/* Test case for successful rb config population*/
TEST_FUNCTION(test_populate_rb_configs_from_rpss_entity, NULL, NULL)
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

    /* Set the owning interface*/
    PDFWK_SYNC_REQUEST_HEADER req = (PDFWK_SYNC_REQUEST_HEADER)&r;
    req->OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;

    mock_pcie_ent.id = r.rpss_index;
    mock_pcie_ent.rps[0].enabled = true;
    mock_pcie_ent.rps[0].valid = true;
    mock_pcie_ent.rps[3].enabled = true;
    mock_pcie_ent.rps[3].valid = true;

    /* Setup silibs expectations */
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    expect_value(__wrap_pciess_config_entity, program_phy_regs, true);
    expect_value(__wrap_pciess_config_entity, enable_apu, true);
    will_return(__wrap_pciess_config_entity, SILIBS_SUCCESS);
    will_return(__wrap_pciess_config_ss_for_bifur, SILIBS_SUCCESS);
    will_return(__wrap_pciess_deassert_por_reset, SILIBS_SUCCESS);
    pcie_sched_sync_op(&(r.header));

    assert_int_equal(rb_configs[0].flags.is_enabled, true);
    assert_int_equal(rb_configs[3].flags.is_enabled, true);
}

/* Test case for initial pciess init sync. request atu mapping failures */
TEST_FUNCTION(test_pcie_rpss_init_atu_map_fail, NULL, NULL)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = INITIAL_CONFIG_REQUEST;
    r.req_type = INITIAL_CONFIG_REQUEST;
    r.rpss_index = RPSS2;

    expect_value(__wrap_atu_map, atu_id, ATU_ID_MSCP);
    will_return(__wrap_atu_map, SILIBS_E_RANGE);
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        pcie_sched_sync_op(&(r.header));
    }
}

TEST_FUNCTION(test_pcie_rpss_pre_rp_ready_init_success, NULL, NULL)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = PRE_RP_INIT_REQUEST;
    r.req_type = PRE_RP_INIT_REQUEST;
    r.rpss_index = RPSS2;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_pciess_phys_sram_init_done, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_pre_rp_ready_init, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_pcie_rpss_post_rp_ready_init_success, NULL, NULL)
{
    /* Setup the request for an rpss */
    pcie_sync_request_t r;
    r.header.RequestType = POST_RP_INIT_REQUEST;
    r.req_type = POST_RP_INIT_REQUEST;
    r.rpss_index = RPSS2;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS2);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return(__wrap_pciess_rps_ready, SILIBS_SUCCESS);
    will_return(__wrap_pciess_rps_post_rp_ready_init, SILIBS_SUCCESS);
    int32_t ret = pcie_sched_sync_op(&(r.header));
    assert_int_equal(ret, 0);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_default_async_dispatch, NULL, NULL)
{
    /* Setup the request */
    pcie_async_request_t r;
    iface.dev = &dev;
    r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
    r.rpss_index = RPSS3;
    r.rp_index = 3;

    expect_value(__wrap_DfwkQueueEnqueueRequest, Queue, &(dev.per_rp_queue[3]));
    expect_value(__wrap_DfwkQueueEnqueueRequest, Request, &(r.header));
    pcie_default_dispatch(&(r.header), nullptr);

    /* Check out of bounds root port index */
    r.rp_index = 50;
    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
    pcie_default_dispatch(&(r.header), nullptr);
    assert_int_equal(r.status, SILIBS_E_PARAM);
}

TEST_FUNCTION(test_link_training_dispatch, NULL, NULL)
{
    /* Setup the request */
    pcie_async_request_t r;
    iface.dev = &dev;
    r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
    r.rpss_index = RPSS3;
    r.rp_index = 3;
    r.rp_op = INITIATE_LINK_TRAINING;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS3);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rp_initiate_link_training, SILIBS_SUCCESS);
    will_return(__wrap__txe_timer_delete, TX_SUCCESS);
    will_return(__wrap__txe_timer_create, TX_SUCCESS);
    pcie_per_rp_dispatch(&(r.header), nullptr);
}

TEST_FUNCTION(test_link_training_threadx_failure, NULL, NULL)
{
    pcie_async_request_t r;
    iface.dev = &dev;
    r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
    r.rpss_index = RPSS3;
    r.rp_index = 3;
    r.rp_op = INITIATE_LINK_TRAINING;

    mock_pcie_ent.id = r.rpss_index;

    expect_value(__wrap_pciess_get_entity, rpss_idx, RPSS3);
    will_return(__wrap_pciess_get_entity, &mock_pcie_ent);
    will_return(__wrap_pciess_rp_initiate_link_training, SILIBS_SUCCESS);
    will_return(__wrap__txe_timer_delete, TX_SUCCESS);
    will_return(__wrap__txe_timer_create, TX_TIMER_ERROR);
    expect_value(FPFwErrorRaise, error, (uint32_t)(TX_TIMER_ERROR));
    if (!set_error_handler_return())
    {
        pcie_per_rp_dispatch(&(r.header), nullptr);
    }
}

TEST_FUNCTION(test_link_training_bad_input, NULL, NULL)
{
    expect_value(FPFwErrorRaise, error, (uint32_t)(-1));
    if (!set_error_handler_return())
    {
        begin_link_training(nullptr);
    }
}

TEST_FUNCTION(test_get_status_dispatch, NULL, NULL)
{
    /* Setup the request */
    pcie_async_request_t r;
    iface.dev = &dev;
    r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
    r.rpss_index = RPSS3;
    r.rp_index = 3;
    r.rp_op = WAIT_FOR_EVENT;

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
    pcie_per_rp_dispatch(&(r.header), nullptr);
    assert_int_equal(r.status, SILIBS_SUCCESS);
}

TEST_FUNCTION(test_unknown_req_dispatch, NULL, NULL)
{
    /* Setup the request */
    pcie_async_request_t r;
    iface.dev = &dev;
    r.header.OwningInterface = (PDFWK_INTERFACE_HEADER)&iface;
    r.rpss_index = RPSS3;
    r.rp_index = 3;
    r.rp_op = PCIE_MAX_ASYNC_REQ;

    expect_value(__wrap_DfwkAsyncRequestComplete, Request, &(r.header));
    pcie_per_rp_dispatch(&(r.header), nullptr);
    assert_int_equal(r.status, SILIBS_E_PARAM);
}
