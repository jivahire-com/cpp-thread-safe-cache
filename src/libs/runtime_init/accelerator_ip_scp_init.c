/**
 * @file accelerator_ip_scp_init.c
 * Initialize the Accelerators on SCP core
 */

/*------------- Includes -----------------*/
#include <accelerator_ip.h> // for scp_accelerators_init
#include <atu_init.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMP...
#include <idsw.h>
#include <idsw_kng.h>
#include <stdio.h> // for printf, NULL

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
// FPFW_INIT_COMPONENT(accel, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "mesh"))
FPFW_INIT_COMPONENT(accel, FPFW_INIT_DEPENDENCIES("vab", "hw_ver", "accel_iso_cfg", "accel_intr_clnt", "nvic", "ddr", "accel_atu", "cd_pomesh"))
{
    // Initialize the Accelerators
    scp_accelerators_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(accel_atu, FPFW_INIT_DEPENDENCIES("vab", "hw_ver", "atu_svc", "nvic", "ddr"))
{
    // Initialize the Accelerators
    accel_atu_config();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

// TODO: WI 1728772 Prior to this read the fuses to know if accelerators should be isolated or not
FPFW_INIT_COMPONENT(accel_iso_cfg, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "tower_cfg"))
{
    int32_t ret = scp_accelerators_isolation_control();

    return (fpfw_init_result_t){ret, NULL};
}
