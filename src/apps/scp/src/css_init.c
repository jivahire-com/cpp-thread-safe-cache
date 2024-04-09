/**
 * @file css_init.c
 * Instantiates CSS
 */

/*------------- Includes -----------------*/
#include <css.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(css_prme, FPFW_INIT_DEPENDENCIES("uart_bm"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    printf("CSS Pre Mesh init, die_num [%d]\n", die_num);

    css_pre_mesh_init(die_num);

    // TODO: System tower should be configured by HSP, until then configure here for SVP
    // ADO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1725718
    if (idsw_get_platform_sdv() == PLATFORM_SVP_SIM)
    {
        printf("Initializing System Control Tower\n");
        css_configure_system_tower(die_num);
        printf("System Control Tower initialization complete.\n");
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(css_pome, FPFW_INIT_DEPENDENCIES("uart_bm", "mesh"))
{
    printf("CSS Post Mesh init\n");

    css_post_mesh_init();
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}