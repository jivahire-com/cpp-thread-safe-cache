//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_rtosmon_init.cpp
 * RTOS monitor init unit tests
 */

/*-------------------------------- Includes ---------------------------------*/
#include <CMockaWrapper.h>
#include <cstdint>

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS
#include <idsw.h>
#include <idsw_kng.h>
#include <rtosmon.h>
#include <systick_update.h>
#include <tx_api.h>
#include <tx_execution_profile.h>

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

EXECUTION_TIME _tx_execution_thread_time_total;
EXECUTION_TIME _tx_execution_isr_time_total;
EXECUTION_TIME _tx_execution_idle_time_total;

uint64_t (*monitoring_time_source)();
uint64_t (*tx_execution_time_source)();

/*--------------------------------- Externs ---------------------------------*/
extern fpfw_init_component_t _fpfw_component_rtosmon;

/*----------------------------- Global Functions ----------------------------*/

idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

uint32_t __wrap_systick_get_emcpu_clock(void)
{
    return mock_type(uint32_t);
}

fpfw_status_t __wrap_rtosmon_init(rtosmon_config_t const* p_rtosmon_config, rtosmon_data_t* p_rtosmon_data)
{
    monitoring_time_source = p_rtosmon_config->monitoring_time_source;
    tx_execution_time_source = p_rtosmon_config->tx_execution_time_source;
    FPFW_UNUSED(p_rtosmon_data);
    return mock_type(fpfw_status_t);
}

uint64_t __wrap_sdm_get_system_timestamp()
{
    return mock_type(uint64_t);
}

ULONG __wrap__tx_time_get(VOID)
{
    return mock_type(ULONG);
}

uint64_t __wrap_get_timestamp_counter_freq(void)
{
    return mock_type(uint64_t);
}

void __wrap_crash_dump_bug_check(uint32_t p0, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    FPFW_UNUSED(p0);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Just return without setjmp/longjmp
}

} // extern "C" {

//
// Tests
//

TEST_FUNCTION(test_rtosmon_init, nullptr, nullptr)
{
    fpfw_init_result_t result;

    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    will_return_count(__wrap_systick_get_emcpu_clock, 1200000000, 2); // 1.2 GHz
    will_return(__wrap_rtosmon_init, FPFW_STATUS_SUCCESS);

    result = _fpfw_component_rtosmon.init_fn();
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);

    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);
    will_return_count(__wrap_systick_get_emcpu_clock, 1200000000, 2); // 1.2 GHz
    will_return(__wrap_rtosmon_init, FPFW_STATUS_SUCCESS);

    result = _fpfw_component_rtosmon.init_fn();
    assert_int_equal(result.status, FPFW_INIT_STATUS_SUCCESS);

    will_return(__wrap__tx_time_get, 5000); // 5000 ticks
    will_return(__wrap__tx_time_get, 5000); // 5000 ticks
    tx_execution_time_source();

    will_return(__wrap__tx_time_get, 5000); // 5000 ticks
    will_return(__wrap__tx_time_get, 5001); // 5001 ticks
    will_return(__wrap__tx_time_get, 5001); // 5001 ticks
    will_return(__wrap__tx_time_get, 5001); // 5001 ticks
    tx_execution_time_source();
}

TEST_FUNCTION(test_rtosmon_init_fail, nullptr, nullptr)
{
    fpfw_init_result_t result;

    will_return(__wrap_idsw_get_cpu_type, CPU_SDM);
    will_return_count(__wrap_systick_get_emcpu_clock, 1200000000, 2); // 1.2 GHz
    will_return(__wrap_rtosmon_init, FPFW_STATUS_FAIL);

    result = _fpfw_component_rtosmon.init_fn();
}
