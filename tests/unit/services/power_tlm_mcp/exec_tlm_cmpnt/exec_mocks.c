//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file exec_mocks.c
 * Mock functions for executive telemetry component
 */

/*------------- Includes -----------------*/

#include <FpFwCMocka.h> // for check_expected_ptr, mock_type, function_called
#include <FpFwLock.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
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

void data_proc_tlm_cmpnt_process_input_data(void)
{
    function_called();
}

void data_proc_tlm_cmpnt_prepare_data_for_inst_sample(void)
{
    function_called();
}

void data_proc_tlm_cmpnt_prepare_data_for_pwr_pkg(void)
{
    function_called();
}

void data_proc_tlm_cmpnt_prepare_data_for_24hr_pkg(void)
{
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

void in_band_tlm_cmpnt_generate_24hr_pkg(void)
{
    function_called();
}

void in_band_tlm_cmpnt_handle_incoming_mts_msgs(void)
{
    function_called();
}

void in_band_tlm_cmpnt_sample_sensor_fifo_dbg_data(void)
{
    function_called();
}

void in_band_tlm_cmpnt_tlm_mode_exit_actions(tlm_operating_mode_t exiting_mode)
{
    FPFW_UNUSED(exiting_mode);
    function_called();
}

void in_band_tlm_cmpnt_tlm_mode_enter_actions(tlm_operating_mode_t entering_mode)
{
    FPFW_UNUSED(entering_mode);
    function_called();
}

bool in_band_tlm_cmpnt_is_any_instantaneous_enabled(void)
{
    return mock_type(bool);
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

FPFW_LOCK_STATE __wrap_FpFwLockAcquire(PFPFW_LOCK Lock)
{
    FPFW_UNUSED(Lock);
    return 0x48;
}

void __wrap_FpFwLockRelease(PFPFW_LOCK Lock, FPFW_LOCK_STATE OldState)
{
    FPFW_UNUSED(Lock);
    FPFW_UNUSED(OldState);
}

UINT __wrap__txe_timer_change(TX_TIMER* timer_ptr, ULONG initial_ticks, ULONG reschedule_ticks)
{
    FPFW_UNUSED(timer_ptr);
    FPFW_UNUSED(initial_ticks);
    FPFW_UNUSED(reschedule_ticks);

    return mock_type(UINT);
}
