//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sds_init.c
 * This file contains the config/initialization for SDS (Shared Data Structure)
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <sds_init.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(sds_svc, FPFW_INIT_DEPENDENCIES("dfwk", "atu_svc", "arsm"))
{
    static sds_service_t sds_service;
    sds_init(&sds_service, fpfw_init_get_handle((void*)"dfwk"));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &sds_service};
}

FPFW_INIT_COMPONENT(sds_int, FPFW_INIT_DEPENDENCIES("sds_svc"))
{
    static sds_service_interface_t sds_interface;
    sds_interface_init(fpfw_init_get_handle("sds_svc"), &sds_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &sds_interface};
}
