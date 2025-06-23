/**
 * @file fuse_init.c
 * Instantiates fuse service
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h> // for BUG_ASSERT_PARAM, BUG_ASSERT
#include <fpfw_init.h>
#if defined(SCP_RUNTIME_INIT)
    #include <fuse_client.h>
    #include <fuse_init.h>
#endif
#include <fuse_utils.h>
#include <idhw.h>
#include <idsw.h>
#include <stdio.h> // for NULL, printf
#include <system_info.h>
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

//
// Only the SCP has to distribute the fuses, while the MCP and the SCP need the
// feature enabled.
//

//
// Fuse Distribution + Fuse Service CLI
//

#if defined(SCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(fuse_pre_mesh, FPFW_INIT_DEPENDENCIES("icc_hspmbx", "icc_d2dmbx"))
{
    fpfw_icc_base_ctx_t* icc_hspmbx_ctx = fpfw_init_get_handle("icc_hspmbx");
    if (icc_hspmbx_ctx == NULL)
    {
        FPFW_DBGPRINT_ERROR(FUSE_NAME "icc_hspmbx is null\n");
        BUG_CHECK(FPFW_INIT_STATUS_E_CYCLE, icc_hspmbx_ctx, "icc_hspmbx");
        return (fpfw_init_result_t){FPFW_INIT_STATUS_E_CYCLE, NULL};
    }

    fpfw_icc_base_ctx_t* icc_d2dmbx_ctx = fpfw_init_get_handle("icc_d2dmbx");
    if (icc_d2dmbx_ctx == NULL)
    {
        FPFW_DBGPRINT_ERROR(FUSE_NAME "icc_d2dmbx is null\n");
        BUG_CHECK(FPFW_INIT_STATUS_E_CYCLE, icc_hspmbx_ctx, "icc_d2dmbx");
        return (fpfw_init_result_t){FPFW_INIT_STATUS_E_CYCLE, NULL};
    }

    fuse_init(icc_hspmbx_ctx, icc_d2dmbx_ctx);
    fuse_print_version();
    platform_fuse_override();

    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP);
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);

    FPFW_DBGPRINT_INFO(FUSE_NAME "SVC pre-mesh init done\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(fuse_post_mesh,
                    FPFW_INIT_DEPENDENCIES("sds_svc", "fuse_pre_mesh", "css_pome", "ddr_pcr", "mesh_stg_2", "vab", "hw_ver"))
{
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);
    FPFW_DBGPRINT_INFO(FUSE_NAME "SVC post-mesh init done\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

//
// Enable the Fuse CLI
//

FPFW_INIT_COMPONENT(cli_fuse, FPFW_INIT_DEPENDENCIES("fuse_post_mesh"))
{
    FPFW_CLI_STATUS status = platform_fuse_init_cli();
    return (fpfw_init_result_t){status, NULL};
}
#endif

//
// Ensure the fuse feature is enabled
//

#if defined(SCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(fuse_en, FPFW_INIT_DEPENDENCIES("fuse_post_mesh"))
#elif defined(MCP_RUNTIME_INIT)
FPFW_INIT_COMPONENT(fuse_en, FPFW_INIT_NULL_NODE)
#endif
{
    fuse_feature_enable(true);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
