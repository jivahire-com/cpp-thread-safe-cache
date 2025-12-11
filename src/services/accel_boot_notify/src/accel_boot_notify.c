//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_boot_notify.c
 * Accelerator Boot Complete service implementation.
 */

/*------------- Includes -----------------*/
#include "accel_boot_notify.h"

#include "accel_boot_notify_events_i.h"

#include <DbgPrint.h>
#include <ErrorHandler.h> // for FPFwErrorRaise
#include <FpFwUtils.h>
#include <IFpFwEventTracingGeneration.h>
#include <accelerator_ip.h>
#include <bug_check.h> // for BUG_CHECK
#include <cmsdk_wd.h>
#include <gpio_lib.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define BOOT_COMPLETE_GPIO GPIO_CTRL_PIN_ID(IOSS_GPIO_0, 4)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static const uint8_t boot_timeout_in_secs = 2;
static TX_TIMER accel_boot_notify_timer;

/*------------- Functions ----------------*/
static void boot_complete_polling_callback(ULONG input)
{
    FPFW_UNUSED(input);

    uint32_t is_set = 0;

    int silib_status = gpio_get_input(BOOT_COMPLETE_GPIO, &is_set);

    if (silib_status == SILIBS_SUCCESS && is_set == 1)
    {
        int32_t accel_status = notify_accelerators_uefi_boot();

        FPFW_ET_LOG(AccelBootNotifyParam, accel_status);

        if (accel_status == ACCEL_RET_SUCCESS)
        {
            UINT status = tx_timer_deactivate(&accel_boot_notify_timer);
            BUG_ASSERT_PARAM(status == TX_SUCCESS, status, 0);
        }
        else
        {
            ACCEL_BOOT_NOTIFY_ET_ERROR_PARAM(ACCEL_BOOT_NOTIFY_ET_TYPE_NOTIFY_UEFI_BOOT_COMPLETE_FAIL, accel_status);
            FPFW_DBGPRINT_ERROR("Error in notifying accelerator the uefi boot complete status");
            BUG_CHECK(KNG_ACCEL_NOTIFY_BOOT_COMPLETE_ERR, 0, 0);
        }
    }
}

KNG_STATUS accel_boot_notify_service(void)
{
    UINT ret = tx_timer_create(&accel_boot_notify_timer,       // timer_ptr
                               "accel_boot_notify_timer",      // name_ptr
                               boot_complete_polling_callback, // expiration_function
                               0,
                               boot_timeout_in_secs * TX_TIMER_TICKS_PER_SECOND, // initial_ticks
                               boot_timeout_in_secs * TX_TIMER_TICKS_PER_SECOND, // reschedule_ticks
                               TX_AUTO_ACTIVATE);                                // auto_activate

    if (ret != TX_SUCCESS)
    {
        ACCEL_BOOT_NOTIFY_ET_ERROR_PARAM(ACCEL_BOOT_NOTIFY_ET_TYPE_TIMER_CREATE_FAIL, ret);
        FPFW_DBGPRINT_ERROR("Failed to create timer, error code: %u", ret);
        return KNG_E_FAIL;
    }

    return ret == TX_SUCCESS ? KNG_SUCCESS : KNG_E_FAIL;
}