/**
 * @file mesh_init.c
 * Instantiates Mesh
 */

/*------------- Includes -----------------*/
#include <fpfw_icc_base.h> // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_init.h>
#include <idhw.h>
#include <mesh.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(mesh, FPFW_INIT_DEPENDENCIES("i3c_controller", "icc_hspmbx", "sysinfo", "icc_d2dmbx"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    printf("Mesh init, die_num: [%u]\n", die_num);

    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    mesh_init(die_num, icc_ctx);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
