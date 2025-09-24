/**
 * @file css_init.c
 * Instantiates CSS
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <css.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(css_prme, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "atu_svc", "cfg_mgr", "debug_print"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    FPFW_DBGPRINT_INFO("CSS Pre Mesh init on die[%d]\n", die_num);

    css_pre_mesh_init(die_num);

    // TODO: System tower should be configured by HSP, until then configure here for SVP
    // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1725718
    if (IS_PLATFORM_SVP())
    {
        css_configure_system_tower(die_num);
        FPFW_DBGPRINT_INFO("System Control Tower Initialized\n");
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(css_pome, FPFW_INIT_DEPENDENCIES("std_io", "accel_iso_cfg", "debug_print", "ift"))
{
    FPFW_DBGPRINT_INFO("CSS Post Mesh init\n");

    css_post_mesh_init();
    FPFW_DBGPRINT_INFO("CSS Post Mesh init done\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}