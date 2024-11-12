/**
 * @file tower_init.c
 * Configures various SOC and SS towers
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_init.h>
#include <idsw.h>
#include <stdint.h>
#include <stdio.h>
#include <tower.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(tower_cfg, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "mesh", "atu_svc", "icc_hspmbx", "sysinfo"))
{
    uint8_t die_num = (uint8_t)idsw_get_die_id();

    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");

    FPFW_RUNTIME_ASSERT(icc_ctx != NULL);

    printf("Tower init, die_num [%d]\n", die_num);

    tower_init(die_num, icc_ctx);

    printf("Tower init done, die_num [%d]\n", die_num);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}