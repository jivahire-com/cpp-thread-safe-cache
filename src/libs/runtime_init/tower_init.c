/**
 * @file tower_init.c
 * Configures various SOC and SS towers
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <idhw.h>
#include <idsw.h>
#include <stdint.h>
#include <stdio.h>
#include <tower.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(tower_cfg, FPFW_INIT_DEPENDENCIES("std_io", "hw_ver", "mesh"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    printf("Tower init, die_num [%d]\n", die_num);

    tower_init(die_num);

    printf("Tower init done, die_num [%d]\n", die_num);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}