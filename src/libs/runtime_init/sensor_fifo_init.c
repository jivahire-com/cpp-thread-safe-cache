//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_init.c 
 * Instantiates Sensor Fifo driver and Service
 */

/*------------- Includes -----------------*/

#include <DfwkThreadXHost.h>     // for PDFWK_THREADX_HOST
#include <fpfw_init.h>           // for FPFW_INIT_STATUS_SUCCESS, fpfw_init_get...
#include <scf_mhu_device.h>
#include <sensor_fifo_cli_service.h>
#include <sensor_fifo_driver_interface.h>
#include <sensor_fifo_service.h>
#include <silibs_mcp_exp_top_regs.h> // IWYU pragma: keep
#include <silibs_mcp_top_regs.h>     // IWYU pragma: keep
#include <silibs_scp_exp_top_regs.h> // IWYU pragma: keep
#include <silibs_scp_top_regs.h>     // IWYU pragma: keep
#include <stddef.h>              // for NULL


/*------------- Typedefs -----------------*/
#if defined (SCP_RUNTIME_INIT)
  #define SCF_MHU_BASE_ADDRESS (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_MHU_ADDRESS)
#elif defined (MCP_RUNTIME_INIT)
  #define SCF_MHU_BASE_ADDRESS (MCP_TOP_MCP_EXP_ADDRESS + MCP_EXP_TOP_SCF_MHU_ADDRESS)
#else
  #error runtime not defined
#endif
/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(sensor_fifo, FPFW_INIT_DEPENDENCIES("dfwk"))
{
    // get driver fwk threadx handle
    fpfw_init_component_id_t dfwk_id = "dfwk";
    PDFWK_THREADX_HOST drvfwk = (PDFWK_THREADX_HOST)fpfw_init_get_handle(dfwk_id);

    static scf_mhu_device_t scf_mhu_device = {0};
    scf_mhu_device_initialize(&scf_mhu_device, &drvfwk->Schedule, SCF_MHU_BASE_ADDRESS);

    static sensor_fifo_driver_interface_t sensor_fifo_driver_interface;
    sensor_fifo_driver_inf_init(&sensor_fifo_driver_interface, (sensor_fifo_device_t*)&scf_mhu_device);

    sensor_fifo_svc_initialize(&sensor_fifo_driver_interface);

    sensor_fifo_cli_svc_initialize(&sensor_fifo_driver_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
