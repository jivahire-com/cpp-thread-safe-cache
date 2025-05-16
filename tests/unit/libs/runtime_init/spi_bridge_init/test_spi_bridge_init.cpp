//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_spi_bridge_init.cpp
 * Tests initialization of SPI Bridge Init
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for TEST_FUNCTION, mock_type, will_return
#include <cstdint>         // for uint32_t

extern "C" {
#include <FpFwUtils.h>
#include <fpfw_init.h>   // for fpfw_init_component_id_t, fpfw_...
#include <fpfw_status.h> // for fpfw_status_t, FPFW_STATUS_SUCCESS
#include <mscp_exp_rmss_memory_map.h>

/*---------------Macros-------------------*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_spi_bridge;

/*------------- Mock Functions ----------------*/

int __wrap_spi_controller_init(uintptr_t spi_master_reg, uint16_t clkDiv)
{
    FPFW_UNUSED(spi_master_reg);
    FPFW_UNUSED(clkDiv);
    return mock_type(int);
}

int __wrap_spi_bridge_init(uintptr_t spi_bridge_reg, uint16_t clkDiv)
{
    FPFW_UNUSED(spi_bridge_reg);
    FPFW_UNUSED(clkDiv);
    return mock_type(int);
}

int __wrap_spi_controller_check_errors(uintptr_t spi_master_reg)
{
    FPFW_UNUSED(spi_master_reg);
    return mock_type(int);
}

int __wrap_spi_bridge_check_errors(uintptr_t spi_bridge_reg)
{
    FPFW_UNUSED(spi_bridge_reg);
    return mock_type(int);
}

int __wrap_spi_bridge_clear_error_interrupts(uintptr_t spi_bridge_reg)
{
    FPFW_UNUSED(spi_bridge_reg);
    return mock_type(int);
}

TEST_FUNCTION(test_spi_bridge_init, nullptr, nullptr)
{
    will_return(__wrap_spi_bridge_init, SILIBS_SUCCESS);
    will_return(__wrap_spi_bridge_check_errors, 1);
    will_return(__wrap_spi_bridge_clear_error_interrupts, SILIBS_SUCCESS);
    will_return(__wrap_spi_controller_init, SILIBS_SUCCESS);
    will_return(__wrap_spi_bridge_check_errors, SILIBS_SUCCESS);
    will_return(__wrap_spi_controller_check_errors, SILIBS_SUCCESS);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_spi_bridge.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
}
}