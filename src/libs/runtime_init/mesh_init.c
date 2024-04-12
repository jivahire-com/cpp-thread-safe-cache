/**
 * @file mesh_init.c
 * Instantiates Mesh
 */

/*------------- Includes -----------------*/
#include <fpfw_init.h>
#include <idhw.h>
#include <mesh.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(mesh, FPFW_INIT_DEPENDENCIES("css_prme"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    printf("Mesh init, die_num: [%u]\n", die_num);

    mesh_init(die_num);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
