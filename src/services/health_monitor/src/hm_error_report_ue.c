//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_report_ue.c
 * Implements uncorrectable error report functionality.
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <bug_check.h>
#include <gicd_regs.h>
#include <gpio_lib.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <idhw.h>
#include <idsw.h>
#include <ras.h>
#include <silibs_platform.h>

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
    // inform to AP
    if (trigger)
    {
        if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
        {
            MMIO_WRITE32(MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR + GICD_GICD_SETSPI_NSR_ADDRESS, OS_CPER_ERROR_DEVICE_EVT);
        }
    }
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
