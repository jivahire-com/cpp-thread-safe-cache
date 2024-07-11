/**
 * @file ddrss_init.c
 * Instantiates DDR Subsystem
 */

/*------------- Includes -----------------*/
#include <ddrss.h>
#include <fpfw_init.h>
#include <idsw_kng.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(ddr, FPFW_INIT_DEPENDENCIES("css_pome"))
{
    KNG_DIE_ID die_num = idsw_get_die_id();
    printf("DDR init, die_num: [%u]\n", die_num);

    prod_ddrss_lib_init(die_num);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
