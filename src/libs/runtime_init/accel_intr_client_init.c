//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_intr_client_init.c
 * This file contains the config/initialization for the Accel Interrupt Client
 */

/*------------- Includes -----------------*/

#include "accel_intr_service_dfwk.h" // for accel_intr_service_interface_t, accel...

#include <accel_intr_client.h> // for accel_intr_client_init
#include <fpfw_init.h>         // for fpfw_init_get_handle, FPFW_INIT_STATU...
#include <stddef.h>            // for NULL
#include <stdio.h>             // for printf, NULL

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(accel_intr_clnt, FPFW_INIT_DEPENDENCIES("accel_intr_inf"))
{
    static accel_intr_service_interface_t* accel_intr_service_interface;

    accel_intr_service_interface = (accel_intr_service_interface_t*)fpfw_init_get_handle("accel_intr_inf");

    printf("accel_intr_clnt : Init start\n");
    accel_intr_client_init(accel_intr_service_interface);
    printf("accel_intr_clnt : Init end\n");

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}