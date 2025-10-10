/**
 * @file tower_init.c
 * Configures various SOC and SS towers
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <bug_check.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_init.h>
#include <idsw.h>
#include <stdint.h>
#include <stdio.h>
#include <tower.h>
#include <tower_isr.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(tower_cfg, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "mesh_stg_1", "atu_svc", "icc_hspmbx", "sysinfo"))
{
    uint8_t die_num = (uint8_t)idsw_get_die_id();

    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");

    BUG_ASSERT(icc_ctx != NULL);

    FPFW_DBGPRINT_INFO("Tower init, die_num [%d]\n", die_num);

    tower_init(die_num, icc_ctx);

    FPFW_DBGPRINT_INFO("Tower init done, die_num [%d]\n", die_num);

    /* Register the error domain for NITower in Health Monitor */
    hm_register_error_domain(ACPI_ERROR_DOMAIN_NITOWER, NULL, "Tower Error Domain", tower_error_injection_cb, NULL);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
