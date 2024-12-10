//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file atu_init.c
 * This file contains the config/initialization for ATU service.
 */

/*------------- Includes -----------------*/
#include <atu_init.h>
#include <fpfw_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(atu_svc, FPFW_INIT_DEPENDENCIES("dfwk", "hw_ver", "std_io"))
{
    static atu_device_t atu_device;
    static atu_service_t atu_service;

    atu_service.atu_device = &atu_device;

    atu_svc_init(&atu_service, fpfw_init_get_handle((void*)"dfwk"));
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &atu_service};
}
