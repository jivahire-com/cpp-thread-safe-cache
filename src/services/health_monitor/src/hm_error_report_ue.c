//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_report_ue.c
 * Implements uncorrectable error report functionality.
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <gpio_lib.h>
#include <health_monitor_i.h>

/*-- Symbolic Constant Macros (defines) --*/
#define FATAL_ERROR_REPORT  0
#define GPIO_HM_FATAL_ERROR GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_6, FATAL_ERROR_REPORT)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void hm_report_error_gpio(bool trigger)
{
    // This signal is active-low
    gpio_set_output(GPIO_HM_FATAL_ERROR, !trigger);
}

void hm_report_error_varsrv(bool trigger)
{
    FPFW_UNUSED(trigger);
}

void hm_report_error_interrupt(bool trigger)
{
    FPFW_UNUSED(trigger);
}

void hm_report_error_event(hm_error_report_type_t type, bool trigger)
{
    switch (type)
    {
    case HM_ERROR_REPORT_GPIO: {
        hm_report_error_gpio(trigger);
        break;
    }

    case HM_ERROR_REPORT_VARSVC: {
        hm_report_error_varsrv(trigger);
        break;
    }
    case HM_ERROR_REPORT_INTERRUPT: {
        hm_report_error_interrupt(trigger);
        break;
    }
    default:
        BUG_ASSERT_PARAM(false, type, 0);
    }
}
