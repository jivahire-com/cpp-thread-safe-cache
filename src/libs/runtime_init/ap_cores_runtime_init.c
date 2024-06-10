/**
 * @file ap_cores_runtime_init.c
 * Initialize the AP Cores Initialization module
 */

/*------------- Includes -----------------*/
#include <ap_cores_init.h>  // for primary_ap_power_on
#include <fpfw_init.h>       // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMP...
#include <stdio.h>           // for printf, NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(apcores, FPFW_INIT_DEPENDENCIES("accel"))
{
    // Initialize the Accelerators
    printf("AP Cores init start!!!\n");
    int32_t ret = scp_ap_cores_init();
    printf("AP Cores init complete!!!\n");

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &ret};
}
