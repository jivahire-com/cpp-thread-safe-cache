//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file exec_mocks.c
 * Mock functions for executive telemetry component
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwUtils.h>  // for FPFW_UNUSED
#include <exec_tlm_cmpnt.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void __wrap_FpFwAssertWithArgs(int expression, uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    FPFW_UNUSED(expression);
    FPFW_UNUSED(arg0);
    FPFW_UNUSED(arg1);
    FPFW_UNUSED(arg2);
    FPFW_UNUSED(arg3);

    function_called();
}

void data_proc_tlm_cmpnt_aggregate_pwr_tlm_data(void)
{
    function_called();
}

void data_proc_tlm_cmpnt_aggregate_inst_tlm_data(void)
{
    function_called();
}

void data_proc_tlm_cmpnt_aggregate_24hr_tlm_data(void)
{
    function_called();
}

void data_proc_tlm_cmpnt_enable_disable_transition(bool enable)
{
    FPFW_UNUSED(enable);

    function_called();
}

void in_band_tlm_cmpnt_add_inst_sample(void)
{
    function_called();
}

void in_band_tlm_cmpnt_generate_pwr_pkg(void)
{
    function_called();
}

void in_band_tlm_cmpnt_handle_incoming_mts_msgs(void)
{
    function_called();
}

UINT __wrap__txe_timer_info_get(TX_TIMER* timer_ptr, CHAR** name, UINT* active, ULONG* remaining_ticks, ULONG* reschedule_ticks, TX_TIMER** next_timer)
{
    FPFW_UNUSED(timer_ptr);
    FPFW_UNUSED(name);
    FPFW_UNUSED(remaining_ticks);
    FPFW_UNUSED(reschedule_ticks);
    FPFW_UNUSED(next_timer);

    *active = mock_type(UINT);
    return mock_type(UINT);
}

uint64_t __wrap_gtimer_prodfw_get_counter(void)
{
    return mock_type(uint64_t);
}

uint32_t __wrap_gtimer_prodfw_get_frequency(void)
{
    return mock_type(uint32_t);
}
