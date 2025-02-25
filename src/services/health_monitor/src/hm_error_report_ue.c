//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_report_ue.c
 * Implements uncorrectable error report functionality.
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <health_monitor_i.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

void hm_report_error_gpio()
{
    // Assert GPIO
}

void hm_report_error_var_srv()
{
    // Variable Service
}

void hm_report_error_INTERRUPT()
{
    // Interrupt to AP
}

void hm_report_uncorrected_error(hm_error_report_type_t type)
{
    switch (type)
    {
    case HM_ERROR_REPORT_GPIO: {
        hm_report_error_gpio();
        break;
    }

    case HM_ERROR_REPORT_VARSVC: {
        hm_report_error_var_srv();
        break;
    }
    case HM_ERROR_REPORT_INTERRUPT: {
        hm_report_error_INTERRUPT();
        break;
    }
    default:
        BUG_ASSERT_PARAM(false, type, 0);
    }
}
