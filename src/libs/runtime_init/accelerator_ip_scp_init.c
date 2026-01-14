//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file accelerator_ip_scp_init.c
 * Initialize the Accelerators on SCP core
 */

/*------------- Includes -----------------*/
#include <accel_intr.h>
#include <accelerator_ip.h> // for scp_accelerators_init
#include <accelip_id.h>     // for ACCEL_ID_CDED, ACCEL_ID_SDM
#include <atu_init.h>
#include <fpfw_init.h> // for FPFW_INIT_STATUS_SUCCESS, FPFW_INIT_COMP...
#include <idsw.h>
#include <idsw_kng.h>
#include <interrupts.h>
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
FPFW_INIT_COMPONENT(
    accel,
    FPFW_INIT_DEPENDENCIES("vab", "hw_ver", "accel_iso_cfg", "accel_intr_clnt", "nvic", "ddr", "accel_atu", "cd_pomesh", "virt_irq", "cfg_mgr", "boot_stat", "sysinfo"))
{
    // Update accel irq numbers used in irq init
    accel_intr_set_irq_num_for_accel(ACCEL_ID_SDM, HW_INT_SDM_COMB_INT);
    accel_intr_set_irq_num_for_accel(ACCEL_ID_CDED, HW_INT_CDED_COMB_INT);

    // Initialize the Accelerators
    scp_accelerators_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(accel_atu, FPFW_INIT_DEPENDENCIES("vab", "hw_ver", "atu_svc", "nvic", "ddr", "cfg_mgr"))
{
    // Initialize the Accelerators
    for (ACCEL_ID accel_type = ACCEL_ID_SDM; accel_type < NUM_VALID_ACCEL_ID; accel_type++)
    {
        if (!accel_is_isolation_enabled(accel_type))
        {
            accel_atu_config(accel_type);
        }
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

// TODO: WI 1728772 Prior to this read the fuses to know if accelerators should be isolated or not
FPFW_INIT_COMPONENT(accel_iso_cfg, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "mesh_stg_2"))
{
    int32_t ret = scp_accelerators_isolation_control();

    return (fpfw_init_result_t){ret, NULL};
}
