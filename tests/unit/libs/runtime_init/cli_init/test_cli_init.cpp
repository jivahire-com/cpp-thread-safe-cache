//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_hw_version_init.cpp
 * Tests the init of the hw version component
 */

// @SSI_Unit_Test
// @SSI_Unit_Test:cli_init

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, assert...
#include <cstdint>         // for int32_t

extern "C" {
#include <FpFwCli.h>      // for _FPFW_CLI_CONFIG, CLI_SUCCESS, PFPFW_CLI_...
#include <FpFwUtils.h>    // for FPFW_UNUSED
#include <fpfw_init.h>    // for fpfw_init_component_t, fpfw_init_get_handle
#include <textio_pl011.h> // for textio_pl011_device_t, textio_pl011_config_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_cli;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const fpfw_init_component_id_t id)
{
    FPFW_UNUSED(id);
    return mock_type(void*);
}

void __wrap_textio_pl011_device_interface_initialize(textio_pl011_device_t* device, textio_pl011_interface_t* pl011_interface)
{
    //! Verify uart handle
    check_expected_ptr(device);
    assert_non_null(pl011_interface);
    function_called();
}

int32_t __wrap_FpFwCliInitialize(PFPFW_CLI_CONFIG pCliConfig)
{
    //! Verify expected cli configuration settings
    assert_non_null(pCliConfig);
    assert_non_null(pCliConfig->CommandHistory);
    assert_int_equal(pCliConfig->MaxCommandLength, 256);
    assert_int_equal(pCliConfig->CommandHistorySize, 4 * 256);
    assert_non_null(pCliConfig->OutputStream);
    assert_non_null(pCliConfig->InputStream);
    //! function expected to be called in init
    function_called();
    return 0;
}

void __wrap_FpFwCliStart()
{
    //! function expected to be called in init
    function_called();
}

//
// Tests
//
TEST_FUNCTION(cli_init, nullptr, nullptr)
{
    //! Set up expectations
    textio_pl011_device_t test_pl011_device = {};
    will_return(__wrap_fpfw_init_get_handle, &test_pl011_device);
    expect_value(__wrap_textio_pl011_device_interface_initialize, device, &test_pl011_device);
    expect_function_call(__wrap_textio_pl011_device_interface_initialize);
    expect_function_call(__wrap_FpFwCliInitialize);
    expect_function_call(__wrap_FpFwCliStart);

    //! Call API under test
    fpfw_init_result_t result = _fpfw_component_cli.init_fn();

    //! Verify expected result
    assert_int_equal(result.status, CLI_SUCCESS);
    assert_null(result.associated_handle);
}
}
