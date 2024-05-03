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
#include <cstddef>         // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep

extern "C" {
#include <DfwkPtrTypes.h>
#include <error_handler.h>
#include <kng_soc_constants.h>
#include <pcie_dfwk.h>
#include <pcie_dfwk_i.h>
#include <silibs_status.h> // for SILIBS_E_PARAM, SILIBS_SUCCESS
#include <tx_api.h>
}

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
pciess_device_t dev;
pciess_device_interface_t iface;

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

TEST_FUNCTION(test_sync_requests, NULL, NULL)
{
    pcie_sync_request_t r;
    r.rpss_index = RPSS2;
    int32_t ret = pcie_sched_sync_op(&(r.header));
    /* For now, any sync. requests return success buy default. */
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
