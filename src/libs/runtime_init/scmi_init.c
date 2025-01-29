//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file scmi_init.c
 * This file contains the config/initialization for the SCMI driver
 */

/*------------- Includes -----------------*/

#include <fpfw_init.h> // for fpfw_init_get_handle, FPFW_INIT_S...
#include <scmi_init.h>
#include <stdio.h> // for printf

// pass ap_core svc interface to SCMI driver
FPFW_INIT_COMPONENT(scmi_drv, FPFW_INIT_DEPENDENCIES("icc_mscp2tfa_if", "ap_core_int", "std_io"))
{
    scmi_drv_init(fpfw_init_get_handle("icc_mscp2tfa_if"));
    scmi_set_apcore_interface(fpfw_init_get_handle("ap_core_int"));
    printf("SCMI Init Complete\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
