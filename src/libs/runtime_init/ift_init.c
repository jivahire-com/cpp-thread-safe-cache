//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file ift_init.c
 * Instantiates IFT
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h>
#include <DfwkThreadXHost.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <ift_fw.h>
#include <interrupts.h>
#include <stdint.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(ift, FPFW_INIT_DEPENDENCIES("icc_hspmbx", "mpu"))
{
    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");

    ift_init(icc_ctx);

    ift_set_skip_irq(HW_INT_HSP2SCP_MB_SW_INT);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(ift_drv, FPFW_INIT_DEPENDENCIES("ift", "dfwk"))
{
    static ift_device_t ift_device;
    static ift_interface_t ift_interface;

    /* Skip IFT driver framework initialization if IFT is disabled. */
    if (!ift_is_enabled())
    {
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle("dfwk");

    ift_dfwk_device_initialize(&ift_device, &(dfwk_host->Schedule));
    ift_dfwk_interface_initialize(&ift_interface, &ift_device);
    ift_dfwk_set_interface(&ift_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &ift_interface};
}
