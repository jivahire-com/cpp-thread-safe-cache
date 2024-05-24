/**
 * @file accelerator_ip_init.c
 * Initialize the Accelerators
 */

/*------------- Includes -----------------*/
#include <accelerator_ip.h>  // for scp_accelerators_init
#include <fpfw_init.h>       // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMP...
#include <stdio.h>           // for printf, NULL

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

/*
 * TODO: ADO: 1775434
 *
 * Enabling the explicit dependency list is causing the `s_init_context.head` in
 * the `fpfw_init()` path as `NULL`. Hence as a fix, we are just calling out the
 * `css_pome` which in-turn depends on the `std_io`, `hw_ver` and `mesh`.
 */
//FPFW_INIT_COMPONENT(accel, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "mesh"))
FPFW_INIT_COMPONENT(accel, FPFW_INIT_DEPENDENCIES("ddr"))
{
    // Initialize the Accelerators
    printf("Accelerator init start!!!\n");
    scp_accelerators_init();
    printf("Accelerator init complete!!!\n");

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

