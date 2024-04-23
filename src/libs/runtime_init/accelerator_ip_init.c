/**
 * @file accelerator_ip_init.c
 * Initialize the Accelerators
 */

/*------------- Includes -----------------*/
#include <accelerator_ip.h>
#include <fpfw_init.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(accel, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "mesh"))
{
    // Initialize the Accelerators
    printf("Accelerator init start!!!\n");
    scp_accelerators_init();
    printf("Accelerator init complete!!!\n");

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
