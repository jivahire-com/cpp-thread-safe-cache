//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_init.c
 * This file contains the config/initialization for the power service
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <power_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(pwr_svc, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    static power_service_t power_service;
    power_init(&power_service, fpfw_init_get_handle((void*)"dfwk"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &power_service};
}

FPFW_INIT_COMPONENT(pwr_int, FPFW_INIT_DEPENDENCIES("pwr_svc"))
{
    static power_service_interface_t power_interface;
    power_interface_init(fpfw_init_get_handle("pwr_dev"), &power_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &power_interface};
}