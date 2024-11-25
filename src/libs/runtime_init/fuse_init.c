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
        printf(FUSE_NAME "Error: Failed to get icc_hspmbx context\n");
        BUG_CHECK(FPFW_INIT_STATUS_E_CYCLE, icc_hspmbx_ctx, "icc_hspmbx");
        return (fpfw_init_result_t){FPFW_INIT_STATUS_E_CYCLE, NULL};
    }

    fuse_init(icc_hspmbx_ctx);
    platform_fuse_override();

    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP);
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);

    printf(FUSE_NAME "SVC pre-mesh init successfully\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(fuse_post_mesh,
                    FPFW_INIT_DEPENDENCIES("sds_svc", "fuse_pre_mesh", "css_pome", "ddr_pcr", "tower_cfg", "vab", "hw_ver"))
{
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);

    // TODO: The fuse distribution erroneously unmaps the ATU mapping
    // MSCP_START_ADDRESS: 60200000    MSCP_END_ADDRESS: 689B1FFF   AP_ADDRESS: 000200000000
    // Restore that mapping
    // ADO: 2199711
    uint64_t ap_base_address = idsw_get_die_id() == DIE_0 ? AP_TOP_D0_CORE_CLUSTER_ADDRESS : AP_TOP_D1_CORE_CLUSTER_ADDRESS;
    atu_map_entry_t atu_entry = {
        .ap_base_address = ap_base_address + 0x4000,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_END_ADDR,
        .attribute = {{.axprot0 = ATU_BUS_ATTR_SET, .axprot1 = ATU_BUS_ATTR_CLR, .axnse = ATU_BUS_ATTR_SET}},
    };
    atu_unmap(ATU_ID_MSCP, &atu_entry);
    atu_entry.ap_base_address = idsw_get_die_id() == DIE_0 ? AP_TOP_D0_CORE_CLUSTER_ADDRESS : AP_TOP_D1_CORE_CLUSTER_ADDRESS;
    atu_map(ATU_ID_MSCP, &atu_entry);

    printf(FUSE_NAME "SVC post-mesh init successfully\n");
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}

FPFW_INIT_COMPONENT(cli_fuse, FPFW_INIT_DEPENDENCIES("fuse_post_mesh"))
{
    FPFW_CLI_STATUS status = platform_fuse_init_cli();
    return (fpfw_init_result_t){status, NULL};
}
