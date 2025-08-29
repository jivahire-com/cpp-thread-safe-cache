//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_power_pldm_mcp_init.cpp
 * Power PLDM MCP init test
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <fpfw_pldm_service.h>
#include <idsw_kng.h>
#include <power_pldm.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern fpfw_init_component_t _fpfw_component_pwr_pldm;
static uint32_t dummy_icc_ctx = 0;

/*------------- Functions ----------------*/
//
// Mocks
//
void* __wrap_fpfw_init_get_handle(const char* id)
{
    FPFW_UNUSED(id);
    return &dummy_icc_ctx;
}

idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

fpfw_status_t __wrap_fpfw_pldm_service_register_numeric_effecter(pldm_numeric_effecter_context_t* p_effecter,
                                                                 pldm_numeric_effecter_config_t* p_config)
{
    FPFW_UNUSED(p_effecter);
    FPFW_UNUSED(p_config);
    return FPFW_STATUS_SUCCESS;
}

fpfw_status_t __wrap_fpfw_pldm_service_register_state_sensor(pldm_state_sensor_context_t* p_sensor,
                                                             pldm_state_sensor_config_t* p_config)
{
    FPFW_UNUSED(p_sensor);
    FPFW_UNUSED(p_config);
    return FPFW_STATUS_SUCCESS;
}

void __wrap_FpFwLockInitialize(PFPFW_LOCK Lock)
{
    FPFW_UNUSED(Lock);
}
} // extern "C"

//
// Tests
//
TEST_FUNCTION(test_power_pldm_mcp_init, nullptr, nullptr)
{
    // Set up expectations
    will_return(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    // Call the function under test
    _fpfw_component_pwr_pldm.init_fn();
}