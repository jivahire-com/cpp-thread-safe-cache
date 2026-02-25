//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file fuse_init.c
 * Instantiates fuse service
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <boot_status.h>
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
FPFW_INIT_COMPONENT(fuse_pre_mesh, FPFW_INIT_DEPENDENCIES("icc_hspmbx", "boot_stat"))
{
    fpfw_icc_base_ctx_t* icc_hspmbx_ctx = fpfw_init_get_handle("icc_hspmbx");
    if (icc_hspmbx_ctx == NULL)
    {
        BUG_CHECK(FPFW_INIT_STATUS_E_CYCLE, icc_hspmbx_ctx, "icc_hspmbx");
    }

    fuse_init(icc_hspmbx_ctx);
    fuse_print_version();
    platform_fuse_override();

    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP);
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_PRE_FUSE_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_PRE_FUSE_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    FPFW_DBGPRINT_INFO(FUSE_NAME "SVC pre-mesh init done\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(fuse_post_mesh,
                    FPFW_INIT_DEPENDENCIES("sds_svc", "fuse_pre_mesh", "css_pome", "ddr_pcr", "mesh_stg_2", "vab", "hw_ver", "icc_die2die", "boot_stat"))
{
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);

    fpfw_icc_base_ctx_t* icc_die2die_ctx = fpfw_init_get_handle("icc_die2die");

    if (!idhw_is_single_die_boot_en() && (icc_die2die_ctx == NULL))
    {
        BUG_CHECK(FPFW_INIT_STATUS_E_CYCLE, icc_die2die_ctx, "icc_die2die");
    }

    fuse_post_mesh_init(icc_die2die_ctx);

    // Apply hardcoded overrides based on sample milestone
    fuse_hardcoded_overrides();

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_POST_FUSE_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_POST_FUSE_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

//
// Apply hardcoded fuse overrides after DDR init is complete
//

FPFW_INIT_COMPONENT(fuse_override, FPFW_INIT_DEPENDENCIES("fuse_post_mesh", "ddr"))
{
    fuse_hardcoded_overrides();
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
