//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_service_init.c
 * This file contains the config/initialization for the Accelerator Interrupt service
 */

/*------------- Includes -----------------*/

#include "accel_intr_service_dfwk.h" // for accel_intr_service_interface_t, accel...

#include <accel_intr_service.h> // for accel_intr_init, accel_intr_interface...
#include <fpfw_init.h>          // for fpfw_init_get_handle, FPFW_INIT_STATU...
#include <stddef.h>             // for NULL
#include <stdio.h>              // for printf, NULL
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

PLACED_CODE FPFW_INIT_COMPONENT(accel_intr_svc, FPFW_INIT_DEPENDENCIES("dfwk", "std_io"))
{
    static accel_intr_service_t accel_intr_service;

    accel_intr_service_init(&accel_intr_service, fpfw_init_get_handle((void*)"dfwk"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &accel_intr_service};
}

PLACED_CODE FPFW_INIT_COMPONENT(accel_intr_inf, FPFW_INIT_DEPENDENCIES("accel_intr_svc"))
{
    static accel_intr_service_interface_t accel_intr_service_interface;

    accel_intr_service_interface_init(fpfw_init_get_handle("accel_intr_svc"), &accel_intr_service_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &accel_intr_service_interface};
}
