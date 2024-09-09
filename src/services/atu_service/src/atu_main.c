//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file atu_main.c
 * Implements the primary driver interface of atu service.
 */

/*------------- Includes -----------------*/
#include "atu_api.h"
#include "atu_init.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <css.h>
#include <idsw.h>
#include <kng_soc_constants.h>
#include <shared_sds_def.h>
#include <silibs_ap_top_regs.h>
#include <silibs_common.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
atu_map_entry_t atu_static_map_die0[] = {
    // ARSM Section : Entire ARSM Space for MSCP
    {
        .ap_base_address = AP_TOP_D0_ARSM_SHARED_SRAM_ADDRESS,
        .mscp_start_address = ATU_AP_ARSM_ADDRESS,
        .mscp_end_address = ALIGN_UP(ATU_AP_ARSM_ADDRESS + AP_TOP_D0_ARSM_SHARED_SRAM_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // CORE_CLUSTER on DIE0
    {
        .ap_base_address = AP_TOP_D0_CORE_CLUSTER_ADDRESS,
        .mscp_start_address = ATU_AP_CORE_CLUSTER_ADDRESS,
        .mscp_end_address = ALIGN_UP(ATU_AP_CORE_CLUSTER_ADDRESS + AP_TOP_D0_CORE_CLUSTER_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {{.axprot0 = ATU_BUS_ATTR_SET, .axprot1 = ATU_BUS_ATTR_CLR, .axnse = ATU_BUS_ATTR_SET}},
    },
    {0}};

atu_map_entry_t atu_static_map_die1[] = {
    // ARSM Section : Entire ARSM Space for MSCP
    {
        .ap_base_address = AP_TOP_D1_ARSM_SHARED_SRAM_ADDRESS,
        .mscp_start_address = ATU_AP_ARSM_ADDRESS,
        .mscp_end_address = ALIGN_UP(ATU_AP_ARSM_ADDRESS + AP_TOP_D1_ARSM_SHARED_SRAM_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // CORE_CLUSTER on DIE1
    {
        .ap_base_address = AP_TOP_D1_CORE_CLUSTER_ADDRESS,
        .mscp_start_address = ATU_AP_CORE_CLUSTER_ADDRESS,
        .mscp_end_address = ALIGN_UP(ATU_AP_CORE_CLUSTER_ADDRESS + AP_TOP_D1_CORE_CLUSTER_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {{.axprot0 = ATU_BUS_ATTR_SET, .axprot1 = ATU_BUS_ATTR_CLR, .axnse = ATU_BUS_ATTR_SET}},
    },
    {0}};

/*------------- Functions ----------------*/

static void atu_service_dispatch_async(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    FPFW_UNUSED(p_context);

    switch (p_request->RequestType)
    {
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

static int32_t atu_service_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER p_request)
{
    // this part is not implemented yet
    switch (p_request->RequestType)
    {
    case ATU_IO_REQUEST_MAP_SYNC: {
        break;
    }
    case ATU_IO_REQUEST_UNMAP_SYNC: {
        break;
    }
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    return DFWK_SUCCESS;
}

void static_atu_config(uint8_t die_num)
{
    atu_map_entry_t* atu_static_global_map = NULL;
    int atu_entry_num = 0;

    if (die_num == 0)
    {
        atu_static_global_map = atu_static_map_die0;
        atu_entry_num = ARRAY_SIZE(atu_static_map_die0);
    }
    else if (die_num == 1)
    {
        atu_static_global_map = atu_static_map_die1;
        atu_entry_num = ARRAY_SIZE(atu_static_map_die1);
    }
    else
    {
        FPFW_RUNTIME_ASSERT(false);
    }

    // Initialize MSCP ATU
    int status = atu_init(ATU_ID_MSCP, atu_static_global_map, atu_entry_num);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, SILIBS_SUCCESS);

    // Initialize SCP DMAC ATU
    status = atu_init(ATU_ID_SCP_EXP, atu_static_global_map, atu_entry_num);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, SILIBS_SUCCESS);

    // Initialize MCP DMAC ATU
    /* SiLibs investigation task: FW Task 792282: ATU: atu_init fails for ATU_ID_MCP_EXP on SVP
       Calling assert_fail_on_line: condition=(rgn_cnt > 0) && (rgn_cnt <= ATU_MAX_ENTRY_NUM), file=C:/src/kng/.externs/repo/silibs/li
        braries/atu/src/atu_lib.c, line=199 --> Stub Function not implemented for emCPU (TODO)
        Bug Check: [-2143748094] File [C:/src/kng/src/services/atu_service/src/atu_main.c:134] P1: [4294967285] P2: [0]
    */
    // status = atu_init(ATU_ID_MCP_EXP, atu_static_global_map, atu_entry_num);
    // BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, SILIBS_SUCCESS);
}

void atu_svc_init(atu_service_t* atu_service, PDFWK_SCHEDULE schedule)
{
    DfwkDeviceInitialize(&(atu_service->atu_device->header), schedule);

    DfwkQueueInitialize(&(atu_service->atu_device->default_queue),
                        &(atu_service->atu_device->header),
                        atu_service_dispatch_async,
                        &(atu_service->atu_device->header),
                        DfwkQueueType_SerializedDispatch);

    DfwkInterfaceInitialize(&atu_service->header,
                            &(atu_service->atu_device->header),
                            &(atu_service->atu_device->default_queue),
                            atu_service_dispatch_sync);

    uint8_t die_num = (uint8_t)idsw_get_die_id();
    static_atu_config(die_num);
}