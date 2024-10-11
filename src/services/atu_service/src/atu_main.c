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
#include <assert.h> // for static_assert
#include <atu_lib.h>
#include <bug_check.h>
#include <css.h>
#include <idhw.h>
#include <idsw.h>
#include <kng_soc_constants.h>
#include <shared_sds_def.h>
#include <silibs_ap_top_regs.h>
#include <silibs_common.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

// total inband telemetry DDR size is split evenly between the two dies and the AP window is sized the same
#define IB_TELEMETRY_DDR_PER_DIE_SIZE       (MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_SIZE)
#define IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR (IB_TELEMETRY_RESERVATION_BASE)
#define IB_TELEMETRY_DDR_TOTAL_AP_END_ADDR  (IB_TELEMETRY_RESERVATION_END)
#define IB_TELEMETRY_DDR_TOTAL_SIZE         (IB_TELEMETRY_DDR_TOTAL_AP_END_ADDR - IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR)

#define IB_TELEMETRY_DDR_DIE_0_AP_BASE_ADDR (IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR)
#define IB_TELEMETRY_DDR_DIE_0_AP_END_ADDR \
    (IB_TELEMETRY_DDR_TOTAL_AP_BASE_ADDR + IB_TELEMETRY_DDR_PER_DIE_SIZE)

#define IB_TELEMETRY_DDR_DIE_1_AP_BASE_ADDR (IB_TELEMETRY_DDR_DIE_0_AP_END_ADDR)
#define IB_TELEMETRY_DDR_DIE_1_AP_END_ADDR  (IB_TELEMETRY_DDR_TOTAL_AP_END_ADDR)

static_assert((IB_TELEMETRY_DDR_TOTAL_SIZE) ==
                  ((IB_TELEMETRY_DDR_DIE_0_AP_END_ADDR - IB_TELEMETRY_DDR_DIE_0_AP_BASE_ADDR) +
                   (IB_TELEMETRY_DDR_DIE_1_AP_END_ADDR - IB_TELEMETRY_DDR_DIE_1_AP_BASE_ADDR)),
              "IB Die DDR sizes do not add up to total DDR size");

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// TODO: Should we only map the section of DIE 0's ARSM we need? We'd map up to where
//       host services takes the rest of it.
// TODO: Do we need to update the attributes to align with the security settings for host services?
//       That would only be a subregion of the ARMS on DIE 0 (possibly DIE 1 as well).
atu_map_entry_t atu_static_map_single_die_die0[] = {
    // ARMS DIE 0
    {
        .ap_base_address = AP_TOP_D0_ARSM_SHARED_SRAM_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // CORE_CLUSTER on DIE0
    {
        .ap_base_address = AP_TOP_D0_CORE_CLUSTER_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_END_ADDR,
        .attribute = {{.axprot0 = ATU_BUS_ATTR_SET, .axprot1 = ATU_BUS_ATTR_CLR, .axnse = ATU_BUS_ATTR_SET}},
    },
    {
        .ap_base_address = IB_TELEMETRY_DDR_DIE_0_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    {0},
};

atu_map_entry_t atu_static_map_dual_die_die0[] = {
    // ARMS DIE 0
    {
        .ap_base_address = AP_TOP_D0_ARSM_SHARED_SRAM_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // ARMS DIE 1
    {
        .ap_base_address = AP_TOP_D1_ARSM_SHARED_SRAM_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // CORE_CLUSTER on DIE0
    {
        .ap_base_address = AP_TOP_D0_CORE_CLUSTER_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_END_ADDR,
        .attribute = {{.axprot0 = ATU_BUS_ATTR_SET, .axprot1 = ATU_BUS_ATTR_CLR, .axnse = ATU_BUS_ATTR_SET}},
    },
    {
        .ap_base_address = IB_TELEMETRY_DDR_DIE_0_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    {0},
};

atu_map_entry_t atu_static_map_single_die_die1[] = {
    // ARMS DIE 1
    {
        .ap_base_address = AP_TOP_D1_ARSM_SHARED_SRAM_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // CORE_CLUSTER on DIE1
    {
        .ap_base_address = AP_TOP_D1_CORE_CLUSTER_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_END_ADDR,
        .attribute = {{.axprot0 = ATU_BUS_ATTR_SET, .axprot1 = ATU_BUS_ATTR_CLR, .axnse = ATU_BUS_ATTR_SET}},
    },
    {
        .ap_base_address = IB_TELEMETRY_DDR_DIE_1_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    {0},
};

atu_map_entry_t atu_static_map_dual_die_die1[] = {
    // ARMS DIE 0
    {
        .ap_base_address = AP_TOP_D0_ARSM_SHARED_SRAM_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_0_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // ARMS DIE 1
    {
        .ap_base_address = AP_TOP_D1_ARSM_SHARED_SRAM_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ARSM_DIE_1_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // CORE_CLUSTER on DIE1
    {
        .ap_base_address = AP_TOP_D1_CORE_CLUSTER_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_END_ADDR,
        .attribute = {{.axprot0 = ATU_BUS_ATTR_SET, .axprot1 = ATU_BUS_ATTR_CLR, .axnse = ATU_BUS_ATTR_SET}},
    },
    {
        .ap_base_address = IB_TELEMETRY_DDR_DIE_1_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    {0},
};

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

    bool single_die = idhw_is_single_die_boot_en();

    if (die_num == 0)
    {
        if (single_die)
        {
            atu_static_global_map = atu_static_map_single_die_die0;
            atu_entry_num = ARRAY_SIZE(atu_static_map_single_die_die0);
        }
        else
        {
            atu_static_global_map = atu_static_map_dual_die_die0;
            atu_entry_num = ARRAY_SIZE(atu_static_map_dual_die_die0);
        }
    }
    else if (die_num == 1)
    {
        if (single_die)
        {
            atu_static_global_map = atu_static_map_single_die_die1;
            atu_entry_num = ARRAY_SIZE(atu_static_map_single_die_die1);
        }
        else
        {
            atu_static_global_map = atu_static_map_dual_die_die1;
            atu_entry_num = ARRAY_SIZE(atu_static_map_dual_die_die1);
        }
    }
    else
    {
        FPFW_RUNTIME_ASSERT(false);
    }

    // Initialize MSCP ATU
    int status = atu_init(ATU_ID_MSCP, atu_static_global_map, atu_entry_num);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, SILIBS_SUCCESS);

    // Initialize SCP DMAC ATU
    if (idsw_get_cpu_type() == CPU_SCP)
    {
        status = atu_init(ATU_ID_SCP_EXP, atu_static_global_map, atu_entry_num);
        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, SILIBS_SUCCESS);
    }
    else if (idsw_get_cpu_type() == CPU_MCP)
    {
        status = atu_init(ATU_ID_MCP_EXP, atu_static_global_map, atu_entry_num);
        BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, SILIBS_SUCCESS);
    }
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