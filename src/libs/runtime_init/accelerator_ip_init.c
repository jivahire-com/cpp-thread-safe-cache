/**
 * @file accelerator_ip_init.c
 * Initialize the Accelerators
 */

/*------------- Includes -----------------*/
#include <accelerator_ip.h>  // for scp_accelerators_init
#include <fpfw_init.h>       // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMP...
#include <idsw.h>
#include <idsw_kng.h>
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
FPFW_INIT_COMPONENT(accel, FPFW_INIT_DEPENDENCIES("vab", "hw_ver", "accel_iso_cfg", "atu_svc","accel_intr_clnt", "nvic", "ddrman"))
{
    // Initialize the Accelerators
    printf("Accelerator init start!!!\n");
    scp_accelerators_init();
    printf("Accelerator init complete!!!\n");

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

// TODO: WI 1728772 Prior to this read the fuses to know if accelerators should be isolated or not
FPFW_INIT_COMPONENT(accel_iso_cfg, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "tower_cfg"))
{
    uint8_t die_num = (uint8_t)idsw_get_die_id();
    printf("Disable accelerator tower isolation, TODO: decide based on fuse & knobs, die_num [%d]\n", die_num);
    // TODO: WI 1943858 SVP doesn't implement iso csrs
    int32_t ret = FPFW_INIT_STATUS_SUCCESS;
    if (idsw_get_platform_sdv() != PLATFORM_SVP_SIM)
    {
        ret = scp_accelerators_isolation_control();
        printf("Accelerators isolation disabled\n");
    }
    return (fpfw_init_result_t){ret, NULL};
}
