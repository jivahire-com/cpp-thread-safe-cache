//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_boot_notify_init.c
 * Initialization of the service that notifies accelerator of boot complete.
 */

/*------------- Includes -----------------*/
#include <accel_boot_notify.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMPONENT
#include <kng_error.h> // for KNG_SUCCESS
#include <stddef.h>    // for NULL
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(boot_notify, FPFW_INIT_DEPENDENCIES("icc_sdm_mbx", "icc_cded_mbx", "sysinfo"))
{
    if (system_info_is_warm_start())
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    accel_boot_notify_service();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}