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
#include <boot_status.h> // for boot_status_notify_extd
#include <fpfw_init.h>   // for fpfw_init_component_id_t, fpfw_...
#include <fpfw_status.h> // for fpfw_status_t, FPFW_STATUS_SUCCESS
#include <idsw.h>
#include <idsw_kng.h>
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

void __wrap_boot_status_notify_extd(boot_status_req_t* p_req_mem, uint32_t boot_status, uint32_t boot_status_ex)
{
    check_expected(boot_status);
    assert_non_null(p_req_mem);
    check_expected(boot_status_ex);

    function_called();
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

TEST_FUNCTION(test_spi_bridge_init, nullptr, nullptr)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;

    will_return(__wrap_spi_bridge_init, SILIBS_SUCCESS);
    will_return(__wrap_spi_bridge_check_errors, 1);
    will_return(__wrap_spi_bridge_clear_error_interrupts, SILIBS_SUCCESS);
    will_return(__wrap_spi_controller_init, SILIBS_SUCCESS);
    will_return(__wrap_spi_bridge_check_errors, SILIBS_SUCCESS);
    will_return(__wrap_spi_controller_check_errors, SILIBS_SUCCESS);

    will_return(__wrap_idsw_get_die_id, test_die);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    uint32_t expected_boot_status_ex =
        GEN_BOOT_STATUS_EX_GENERIC_CODE(COMPONENT_GROUP_SCP, MSCP_GENERIC, (test_die == DIE_0) ? SCP_PRIMARY : SCP_SECONDARY);
    expect_value(__wrap_boot_status_notify_extd, boot_status, MSCP_BOOT_STATUS_CODE_SCP_SPI_INIT_END);
    expect_value(__wrap_boot_status_notify_extd, boot_status_ex, expected_boot_status_ex);
    expect_function_call(__wrap_boot_status_notify_extd);

    // Call the function under test
    fpfw_init_result_t result = _fpfw_component_spi_bridge.init_fn();

    // Perform necessary assertions on result
    assert_true(result.status == FPFW_STATUS_SUCCESS);
}
}