/**
 * @file mesh_init.c
 * Instantiates Mesh
 */

/*------------- Includes -----------------*/
#include <fpfw_icc_base.h>   // for fpfw_icc_base_init, fpfw_icc_ba...
#include <fpfw_icc_base_i.h> // for fpfw_icc_base_ctx_t
#include <fpfw_init.h>
#include <fpfw_mbox_icc_transport.h> // for ICC_MBX_ASYNC_RECV, ICC_MBX_ASY...
#include <idhw.h>
#include <mesh.h>
#include <stdint.h>
#include <stdio.h>

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(mesh, FPFW_INIT_DEPENDENCIES("css_prme", "cd_init", "icc_hspmbx", "hw_ver"))
{
    uint8_t die_num = (uint8_t)idhw_get_die_id();
    printf("Mesh init, die_num: [%u]\n", die_num);

    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    FPFW_MBX_PRIMITIVE_CTX* p_mbox_prim_ctx =
        &((fpfw_mbox_icc_transport_device_t*)icc_ctx->icc_cfg.transport_interface->OwningDevice)->mbox_prim_ctx;
    mesh_init(die_num, p_mbox_prim_ctx);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
