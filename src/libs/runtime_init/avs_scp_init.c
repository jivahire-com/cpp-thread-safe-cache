/**
 * @file avs_scp_init.c
 * Instantiates AVS for the SCP
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <scp_avs_driver.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

// TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1743391. Add config for each AVS below.
FPFW_INIT_COMPONENT(avs0, FPFW_INIT_DEPENDENCIES("dfwk", "std_io"))
{
    static scp_avs_device_t avs_device;
    avs_device.avs_bus_num = AVS_BUS0;

    fpfw_init_component_id_t dfwk_id = "dfwk";
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle(dfwk_id);

    scp_avs_init(&avs_device, &dfwk_host->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_device};
}

FPFW_INIT_COMPONENT(avs0_int, FPFW_INIT_DEPENDENCIES("avs0"))
{
    static scp_avs_interface_t avs_interface = {};
    fpfw_init_component_id_t avs_id = "avs0";

    scp_avs_interface_initialize(fpfw_init_get_handle(avs_id), &avs_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_interface};
}

FPFW_INIT_COMPONENT(avs1, FPFW_INIT_DEPENDENCIES("dfwk", "std_io"))
{
    static scp_avs_device_t avs_device;
    avs_device.avs_bus_num = AVS_BUS1;

    fpfw_init_component_id_t dfwk_id = "dfwk";
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle(dfwk_id);

    scp_avs_init(&avs_device, &dfwk_host->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_device};
}

FPFW_INIT_COMPONENT(avs1_int, FPFW_INIT_DEPENDENCIES("avs1"))
{
    static scp_avs_interface_t avs_interface = {};
    fpfw_init_component_id_t avs_id = "avs1";

    scp_avs_interface_initialize(fpfw_init_get_handle(avs_id), &avs_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_interface};
}

FPFW_INIT_COMPONENT(avs2, FPFW_INIT_DEPENDENCIES("dfwk", "std_io"))
{
    static scp_avs_device_t avs_device;
    avs_device.avs_bus_num = AVS_BUS2;

    fpfw_init_component_id_t dfwk_id = "dfwk";
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle(dfwk_id);

    scp_avs_init(&avs_device, &dfwk_host->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_device};
}

FPFW_INIT_COMPONENT(avs2_int, FPFW_INIT_DEPENDENCIES("avs2"))
{
    static scp_avs_interface_t avs_interface = {};
    fpfw_init_component_id_t avs_id = "avs2";

    scp_avs_interface_initialize(fpfw_init_get_handle(avs_id), &avs_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_interface};
}

FPFW_INIT_COMPONENT(avs3, FPFW_INIT_DEPENDENCIES("dfwk", "std_io"))
{
    static scp_avs_device_t avs_device;
    avs_device.avs_bus_num = AVS_BUS3;

    fpfw_init_component_id_t dfwk_id = "dfwk";
    DFWK_THREADX_HOST* dfwk_host = (DFWK_THREADX_HOST*)fpfw_init_get_handle(dfwk_id);

    scp_avs_init(&avs_device, &dfwk_host->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_device};
}

FPFW_INIT_COMPONENT(avs3_int, FPFW_INIT_DEPENDENCIES("avs3"))
{
    static scp_avs_interface_t avs_interface = {};
    fpfw_init_component_id_t avs_id = "avs3";

    scp_avs_interface_initialize(fpfw_init_get_handle(avs_id), &avs_interface);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &avs_interface};
}
