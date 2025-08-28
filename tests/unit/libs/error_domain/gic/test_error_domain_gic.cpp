//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 @file test_error_domain_gic.cpp
 * Tests the gic error domain
 */
/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {

#include <FpFwUtils.h> // for FPFW_UNUSED
#include <atu_api.h>
#include <atu_lib.h>
#include <cper.h>
#include <error_domain_gic.h>
#include <health_monitor.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <shared_sram_ecc_ras_registers_regs.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
hm_error_injection_cb_t g_err_inject_cb = NULL;

/*------------- Functions ----------------*/
//
// Mocks
//
void __wrap_hm_register_error_domain(uint16_t error_domain_idx,
                                     const guid_t* error_domain_guid,
                                     const char* error_domain_name,
                                     hm_error_injection_cb_t err_inject_cb,
                                     void* err_inject_ctx)
{
    assert_true(error_domain_idx == ACPI_ERROR_DOMAIN_GIC);
    assert_true(err_inject_cb != NULL);
    assert_true(error_domain_guid != NULL);
    assert_true(error_domain_name != NULL);
    FPFW_UNUSED(err_inject_ctx);

    function_called();

    g_err_inject_cb = err_inject_cb;
}

void __wrap_mmio_write64(volatile uint32_t* addr, uint32_t data)
{
    check_expected(addr);
    FPFW_UNUSED(data);
    function_called();
}

int __wrap_atu_translate_address(atu_id_t atu_id, uint64_t ap_addr, uint32_t* mscp_addr)
{
    check_expected(atu_id);
    check_expected(ap_addr);
    assert_non_null(mscp_addr);
    // return a value
    *mscp_addr = mock_type(uint32_t);
    return mock_type(int);
}
}

//
// Tests
//

TEST_FUNCTION(test_register_gic_error_domain, nullptr, nullptr)
{
    expect_function_call(__wrap_hm_register_error_domain);
    register_gic_error_domain();
}

TEST_FUNCTION(test_gic_error_injection_invalid, nullptr, nullptr)
{
    ras_einj_info_t einj_payload = {};
    einj_payload.component_group = ACPI_ERROR_DOMAIN_GIC;
    einj_payload.param_type.address_32 = AP_TOP_D0_GIC_SIZE + 1;

    acpi_einj_cmd_status_t status = g_err_inject_cb(&einj_payload, NULL);
    assert_int_not_equal(status, ACPI_EINJ_SUCCESS);
}

TEST_FUNCTION(test_gic_error_injection_valid, nullptr, nullptr)
{
    ras_einj_info_t einj_payload = {};
    einj_payload.component_group = ACPI_ERROR_DOMAIN_GIC;
    einj_payload.param_type.address_32 = 0xea00;

    expect_value(__wrap_atu_translate_address, atu_id, ATU_ID_MSCP);
    expect_value(__wrap_atu_translate_address, ap_addr, AP_TOP_D0_GIC_ADDRESS);

    will_return(__wrap_atu_translate_address, MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR);
    will_return(__wrap_atu_translate_address, 0);

    expect_value(__wrap_mmio_write64, addr, MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + einj_payload.param_type.address_32);
    expect_function_call(__wrap_mmio_write64);

    acpi_einj_cmd_status_t status = g_err_inject_cb(&einj_payload, NULL);
    assert_int_equal(status, ACPI_EINJ_SUCCESS);
}