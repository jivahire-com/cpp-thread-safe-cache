//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_pex_rng.cpp
 * pex_rng tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <pex_rng.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <core_cluster_top_regs.h>
#include <corebits.h>
#include <cper.h>
#include <pex_regs.h>
#include <rng.h>
#include <silibs_ap_top_regs.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern void pex_async_dispatch(PDFWK_ASYNC_REQUEST_HEADER request, void* context);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_FpFwAssert(int expression)
{
    check_expected(expression);
}

void __wrap_rng_enable_r(uint32_t base, uint8_t div)
{
    check_expected(base);
    check_expected(div);
}
void __wrap_rng_disable_r(uint32_t base)
{
    check_expected(base);
}

uint32_t __wrap_rng_wait_for_rng_complete_r(uint32_t base)
{
    check_expected(base);
    return mock_type(uint32_t);
}
void* __wrap_fpfw_init_get_handle(void* name)
{
    check_expected(name);
    return mock_ptr_type(void*);
}

void __wrap_DfwkDeviceInitialize(void* device_header, void* handle)
{
    check_expected(device_header);
    check_expected(handle);
}

void __wrap_DfwkInterfaceInitialize(void* interface_header, void* device_header, void* queue, void* context)
{
    check_expected(interface_header);
    check_expected(device_header);
    check_expected(queue);
    check_expected(context);
}

void __wrap_DfwkAsyncRequestInitialize(void* request_header, size_t size)
{
    check_expected(request_header);
    check_expected(size);
}
void __wrap_DfwkQueueInitialize(void* queue, void* device_header, void* dispatch_function, void* context, int queue_type)
{
    check_expected(queue);
    check_expected(device_header);
    check_expected(dispatch_function);
    check_expected(context);
    check_expected(queue_type);
}

void __wrap_DfwkClientInterfaceOpen(void* interface_header)
{
    check_expected(interface_header);
}
void __wrap_DfwkAsyncRequestComplete(void* request)
{
    check_expected(request);
}

void __wrap_hm_submit_cper(uint16_t error_domain, uint8_t severity, void* cper_section, size_t section_size)
{
    check_expected(error_domain);
    check_expected(severity);
    check_expected(cper_section);
    check_expected(section_size);
}

//
// Tests
//
TEST_FUNCTION(test_init_pex_rng_success, nullptr, nullptr)
{
    const corebits_t test_platform_cores = (corebits_t)COREBITS_INIT_UINT32(0x00000001, 0x00000000, 0x0);
    pex_rng_config_t test_config = {.cluster_pex_base = 0,
                                    .cluster_stride = 0x1000,
                                    .platform_cores_in_die = &test_platform_cores,
                                    .core_count = 1};

    expect_value(__wrap_rng_enable_r, base, PEX_RNG_ADDRESS);
    expect_value(__wrap_rng_enable_r, div, 0xC0);
    expect_value(__wrap_rng_wait_for_rng_complete_r, base, PEX_RNG_ADDRESS);
    will_return(__wrap_rng_wait_for_rng_complete_r, 0x0);

    // DFWK initialization mocks
    expect_value(__wrap_fpfw_init_get_handle, name, (void*)"dfwk");
    will_return(__wrap_fpfw_init_get_handle, (void*)0x12345678);
    expect_any(__wrap_DfwkDeviceInitialize, device_header);
    expect_value(__wrap_DfwkDeviceInitialize, handle, (void*)0x12345678);

    expect_any(__wrap_DfwkQueueInitialize, queue);
    expect_any(__wrap_DfwkQueueInitialize, device_header);
    expect_any(__wrap_DfwkQueueInitialize, dispatch_function);
    expect_any(__wrap_DfwkQueueInitialize, context);
    expect_any(__wrap_DfwkQueueInitialize, queue_type);

    expect_any(__wrap_DfwkInterfaceInitialize, interface_header);
    expect_any(__wrap_DfwkInterfaceInitialize, device_header);
    expect_any(__wrap_DfwkInterfaceInitialize, queue);
    expect_value(__wrap_DfwkInterfaceInitialize, context, NULL);
    expect_any(__wrap_DfwkClientInterfaceOpen, interface_header);

    expect_any(__wrap_DfwkAsyncRequestInitialize, request_header);
    expect_value(__wrap_DfwkAsyncRequestInitialize, size, sizeof(pex_rng_request_t));

    init_pex_rng(&test_config);
}

TEST_FUNCTION(test_reset_pex_rng, nullptr, nullptr)
{
    expect_value(__wrap_rng_disable_r, base, PEX_RNG_ADDRESS);
    expect_value(__wrap_rng_enable_r, base, PEX_RNG_ADDRESS);
    expect_value(__wrap_rng_enable_r, div, 0xC0);
    expect_value(__wrap_rng_wait_for_rng_complete_r, base, PEX_RNG_ADDRESS);
    will_return(__wrap_rng_wait_for_rng_complete_r, 0x0);

    reset_pex_rng(PEX_RNG_ADDRESS);
}

TEST_FUNCTION(test_pex_async_dispatch_send_cper_success, nullptr, nullptr)
{
    // Setup test configuration
    const uint32_t test_cluster_base = 0x10000000;
    pex_rng_config_t test_config = {.cluster_pex_base = test_cluster_base,
                                    .cluster_stride = 0x1000,
                                    .platform_cores_in_die = nullptr,
                                    .core_count = 4};

    // Create mock request header
    pex_rng_request_t mock_request;
    mock_request.Header.RequestType = PEX_SEND_CPER;

    // Expected values for CPER submission
    const uint16_t expected_error_domain = ACPI_ERROR_DOMAIN_PEX;
    const uint8_t expected_severity = ACPI_ERROR_SEVERITY_CORRECTED;
    const size_t expected_cper_size = sizeof(acpi_cper_section_t);

    // Setup mock expectations
    expect_value(__wrap_hm_submit_cper, error_domain, expected_error_domain);
    expect_value(__wrap_hm_submit_cper, severity, expected_severity);
    expect_any(__wrap_hm_submit_cper, cper_section);
    expect_value(__wrap_hm_submit_cper, section_size, expected_cper_size);

    expect_value(__wrap_DfwkAsyncRequestComplete, request, &mock_request.Header);

    // Call the function under test
    pex_async_dispatch(&mock_request.Header, &test_config);
}
}