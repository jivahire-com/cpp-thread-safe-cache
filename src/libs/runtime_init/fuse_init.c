/**
 * @file fuse_init.c
 * Instantiates fuse service
 */

/*------------- Includes -----------------*/
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h> // for BUG_ASSERT_PARAM, BUG_ASSERT
#include <fpfw_init.h>
#include <fuse_client.h>
#include <fuse_init.h>
#include <idhw.h>
#include <idsw.h>
#include <stdio.h> // for NULL, printf
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(fuse_pre_mesh, FPFW_INIT_DEPENDENCIES("icc_hspmbx"))
{
    fpfw_icc_base_ctx_t* icc_hspmbx_ctx = fpfw_init_get_handle("icc_hspmbx");
    if (icc_hspmbx_ctx == NULL)
    {
        printf(FUSE_NAME "icc_hspmbx is null\n");
        BUG_CHECK(FPFW_INIT_STATUS_E_CYCLE, icc_hspmbx_ctx, "icc_hspmbx");
        return (fpfw_init_result_t){FPFW_INIT_STATUS_E_CYCLE, NULL};
    }

    fuse_init(icc_hspmbx_ctx);
    platform_fuse_override();

    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP);
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);

    printf(FUSE_NAME "SVC pre-mesh init done\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(fuse_post_mesh,
                    FPFW_INIT_DEPENDENCIES("sds_svc", "fuse_pre_mesh", "css_pome", "ddr_pcr", "tower_cfg", "vab", "hw_ver"))
{
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);

    printf(FUSE_NAME "SVC post-mesh init done\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cli_fuse, FPFW_INIT_DEPENDENCIES("fuse_post_mesh"))
{
    FPFW_CLI_STATUS status = platform_fuse_init_cli();
    return (fpfw_init_result_t){status, NULL};
}
