/**
 * @file ddrss_init.c
 * Instantiates DDR Subsystem
 */

/*------------- Includes -----------------*/
#include <ddrss.h>
#include <fpfw_init.h>
#include <idhw.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(ddr, FPFW_INIT_DEPENDENCIES("css_pome"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    printf("DDR init, die_num: [%u]\n", die_num);

    ddrss_lib_init(die_num);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
