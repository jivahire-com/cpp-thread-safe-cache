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
#include <accelip_id.h>
#include <assert.h> // for static_assert
#include <atu_lib.h>
#include <bug_check.h>
#include <css.h>
#include <idhw.h>
#include <idsw.h>
#include <kng_atu_mappings.h>
#include <kng_soc_constants.h>
#include <shared_sds_def.h>
#include <silibs_ap_top_regs.h>
#include <silibs_common.h>
#include <silibs_kng_soc.h>
#include <silibs_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <vab_cded_ioss_top_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

// Variable Service DDR space will be shared by primary and secondary die. The AP window is sized the same for both dies.
#define VAR_SVC_DDR_PER_DIE_SIZE       (MSCP_ATU_AP_WINDOW_VAR_SVC_DIE_SIZE)
#define VAR_SVC_DDR_TOTAL_AP_BASE_ADDR (MSCP_VAR_SVC_PAYLOADS_RESERVATION_BASE)
#define VAR_SVC_DDR_TOTAL_AP_END_ADDR  (MSCP_VAR_SVC_PAYLOADS_RESERVATION_END)
#define VAR_SVC_DDR_TOTAL_SIZE         (VAR_SVC_DDR_TOTAL_AP_END_ADDR - VAR_SVC_DDR_TOTAL_AP_BASE_ADDR)

#define VAR_SVC_DDR_DIE_0_AP_BASE_ADDR (VAR_SVC_DDR_TOTAL_AP_BASE_ADDR)
#define VAR_SVC_DDR_DIE_0_AP_END_ADDR  (VAR_SVC_DDR_DIE_0_AP_BASE_ADDR + VAR_SVC_DDR_PER_DIE_SIZE)

#define VAR_SVC_DDR_DIE_1_AP_BASE_ADDR (VAR_SVC_DDR_DIE_0_AP_END_ADDR)
#define VAR_SVC_DDR_DIE_1_AP_END_ADDR  (VAR_SVC_DDR_DIE_1_AP_BASE_ADDR + VAR_SVC_DDR_PER_DIE_SIZE)

// Crash dump region is shared by primary and secondary die. The AP window is sized the same for both dies.
#define CRASH_DUMP_DDR_HEADER_SIZE  (MSCP_ATU_AP_WINDOW_CRASH_DUMP_HEADER_SIZE)
#define CRASH_DUMP_DDR_PER_DIE_SIZE (MSCP_ATU_AP_WINDOW_CRASH_DUMP_DIE_SIZE)

#define CRASH_DUMP_DDR_TOTAL_AP_BASE_ADDR (CRASH_DUMP_RESERVATION_BASE)
#define CRASH_DUMP_DDR_TOTAL_AP_END_ADDR  (CRASH_DUMP_RESERVATION_END)
#define CRASH_DUMP_DDR_TOTAL_SIZE         (CRASH_DUMP_DDR_TOTAL_AP_END_ADDR - CRASH_DUMP_DDR_TOTAL_AP_BASE_ADDR)

#define CRASH_DUMP_DDR_HEADER_AP_BASE_ADDR (CRASH_DUMP_DDR_TOTAL_AP_BASE_ADDR)
#define CRASH_DUMP_DDR_HEADER_AP_END_ADDR  (CRASH_DUMP_DDR_TOTAL_AP_BASE_ADDR + CRASH_DUMP_DDR_HEADER_SIZE)

#define CRASH_DUMP_DDR_DIE_0_AP_BASE_ADDR (CRASH_DUMP_DDR_HEADER_AP_END_ADDR)
#define CRASH_DUMP_DDR_DIE_0_AP_END_ADDR  (CRASH_DUMP_DDR_DIE_0_AP_BASE_ADDR + CRASH_DUMP_DDR_PER_DIE_SIZE)

#define CRASH_DUMP_DDR_DIE_1_AP_BASE_ADDR (CRASH_DUMP_DDR_DIE_0_AP_END_ADDR)
#define CRASH_DUMP_DDR_DIE_1_AP_END_ADDR  (CRASH_DUMP_DDR_TOTAL_AP_END_ADDR)

#define ATU_MAPPING_AP_SDM_MMIO_BASE(die_id) \
    ATU_MAPPING((die_id == 0 ? D0_SDM_PF_CFG_START : D1_SDM_PF_CFG_START), 0, D0_SDM_PF_CFG_SIZE, {ATU_BUS_ATTR_NS})

#define ATU_MAPPING_AP_CDED_MMIO_BASE(die_id) \
    ATU_MAPPING((die_id == 0 ? D0_CDED_PF_CFG_START : D1_CDED_PF_CFG_START), 0, D0_CDED_PF_CFG_SIZE, {ATU_BUS_ATTR_NS})

static_assert((IB_TELEMETRY_DDR_TOTAL_SIZE) ==
                  ((IB_TELEMETRY_DDR_DIE_0_AP_END_ADDR - IB_TELEMETRY_DDR_DIE_0_AP_BASE_ADDR) +
                   (IB_TELEMETRY_DDR_DIE_1_AP_END_ADDR - IB_TELEMETRY_DDR_DIE_1_AP_BASE_ADDR)),
              "IB Die DDR sizes do not add up to total DDR size");

static_assert((VAR_SVC_DDR_TOTAL_SIZE) == ((VAR_SVC_DDR_DIE_0_AP_END_ADDR - VAR_SVC_DDR_DIE_0_AP_BASE_ADDR) +
                                           (VAR_SVC_DDR_DIE_1_AP_END_ADDR - VAR_SVC_DDR_DIE_1_AP_BASE_ADDR)),
              "VAR SVC Die DDR sizes do not add up to total DDR size");

static_assert(MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR <= IB_TELEMETRY_RESERVATION_END,
              "alignment past end of physical memory");
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
    // In Band Telemetry Payloads
    {
        .ap_base_address = IB_TELEMETRY_DDR_DIE_0_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // ICC via MHU Payloads
    {
        .ap_base_address = ICC_MHU_PAYLOADS_RESERVATION_BASE,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ICC_MHU_PAYLOAD_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ICC_MHU_PAYLOAD_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // VAR_SVC on DIE0
    {
        .ap_base_address = VAR_SVC_DDR_DIE_0_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_VAR_SVC_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_VAR_SVC_END_ADDR,
        .attribute = {ATU_BUS_ATTR_S},
    },
    // Crash dump status header on DIE0
    {
        .ap_base_address = CRASH_DUMP_DDR_HEADER_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_HEADER_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_HEADER_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // Crash dump on DIE0
    {
        .ap_base_address = CRASH_DUMP_DDR_DIE_0_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // IOSS region on DIE0
    {
        .ap_base_address = AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IOSS_D0_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IOSS_D0_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // CHBCR for CXL
    {
        .ap_base_address = CHBCR_RESERVATION_BASE,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CHBCR_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CHBCR_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // AP GIC region for RAS notification
    {
        .ap_base_address = AP_TOP_D0_GIC_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_GIC_GICD_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // AP Non-Secure UART for MCP to access Die 0 UART
    {
        .ap_base_address = AP_TOP_D0_AP_UART_NS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_UART_NS_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_UART_NS_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // Cstate timestamps region
    {
        .ap_base_address = AP_CSTATE_BUFFER_DDR_DIE_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CSTATE_TIMESTAMP_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CSTATE_TIMESTAMP_DIE_END_ADDR,
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
    // In Band Telemetry Payloads
    {
        .ap_base_address = IB_TELEMETRY_DDR_DIE_0_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // ICC via MHU Payloads
    {
        .ap_base_address = ICC_MHU_PAYLOADS_RESERVATION_BASE,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ICC_MHU_PAYLOAD_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ICC_MHU_PAYLOAD_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // VAR_SVC on DIE0
    {
        .ap_base_address = VAR_SVC_DDR_DIE_0_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_VAR_SVC_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_VAR_SVC_END_ADDR,
        .attribute = {ATU_BUS_ATTR_S},
    },
    // Crash dump status header on DIE0
    {
        .ap_base_address = CRASH_DUMP_DDR_HEADER_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_HEADER_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_HEADER_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // Crash dump on DIE0
    {
        .ap_base_address = CRASH_DUMP_DDR_DIE_0_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    /**
     * For the D2D send frames, we need to map the send frame on the other die.
     * RMSS HAS section 25.1.1.
     */
    // SCP<->SCP Send Frame
    {
        .ap_base_address = AP_TOP_D1_SCP1R2SCP_MHU_SND_NS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ICC_MHU_SCP2SCP_SEND_FRAME_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ICC_MHU_SCP2SCP_SEND_FRAME_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // MCP<->MCP Send Frame
    {
        .ap_base_address = AP_TOP_D1_MCP1R2MCP_MHU_SND_NS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ICC_MHU_MCP2MCP_SEND_FRAME_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ICC_MHU_MCP2MCP_SEND_FRAME_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // IOSS region on DIE0
    {
        .ap_base_address = AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IOSS_D0_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IOSS_D0_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // IOSS region on DIE1
    {
        .ap_base_address = AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IOSS_D1_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IOSS_D1_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // CHBCR for CXL
    {
        .ap_base_address = CHBCR_RESERVATION_BASE,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CHBCR_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CHBCR_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // AP GIC region for RAS notification
    {
        .ap_base_address = AP_TOP_D0_GIC_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_GIC_GICD_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // AP Non-Secure UART for MCP to access Die 0 UART
    {
        .ap_base_address = AP_TOP_D0_AP_UART_NS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_UART_NS_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_UART_NS_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // Cstate timestamps region
    {
        .ap_base_address = AP_CSTATE_BUFFER_DDR_DIE_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CSTATE_TIMESTAMP_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CSTATE_TIMESTAMP_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    {0},
};

atu_map_entry_t atu_map_accel_die0[NUM_VALID_ACCEL_ID] = {
    // SDMSS on DIE0
    [ACCEL_ID_SDM] = ATU_MAPPING_SDMSS_BASE(SOC_D0),
    // CDEDSS on DIE0
    [ACCEL_ID_CDED] = ATU_MAPPING_CDEDSS_BASE(SOC_D0),
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
    // In Band Telemetry Payloads
    {
        .ap_base_address = IB_TELEMETRY_DDR_DIE_1_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // ICC via MHU Payloads
    {
        .ap_base_address = ICC_MHU_PAYLOADS_RESERVATION_BASE,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ICC_MHU_PAYLOAD_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ICC_MHU_PAYLOAD_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // VAR_SVC on DIE1
    {
        .ap_base_address = VAR_SVC_DDR_DIE_1_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_VAR_SVC_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_VAR_SVC_END_ADDR,
        .attribute = {ATU_BUS_ATTR_S},
    },
    // Crash dump status header on DIE1
    {
        .ap_base_address = CRASH_DUMP_DDR_HEADER_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_HEADER_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_HEADER_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // Crash dump on DIE1
    {
        .ap_base_address = CRASH_DUMP_DDR_DIE_1_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // IOSS region on DIE1
    {
        .ap_base_address = AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IOSS_D1_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IOSS_D1_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // CHBCR for CXL
    {
        .ap_base_address = CHBCR_RESERVATION_BASE,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CHBCR_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CHBCR_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // AP GIC region for RAS notification
    {
        .ap_base_address = AP_TOP_D0_GIC_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_GIC_GICD_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // Cstate timestamps region
    {
        .ap_base_address = AP_CSTATE_BUFFER_DDR_DIE_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CSTATE_TIMESTAMP_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CSTATE_TIMESTAMP_DIE_END_ADDR,
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
    // In Band Telemetry Payloads
    {
        .ap_base_address = IB_TELEMETRY_DDR_DIE_1_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // ICC via MHU Payloads
    {
        .ap_base_address = ICC_MHU_PAYLOADS_RESERVATION_BASE,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ICC_MHU_PAYLOAD_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ICC_MHU_PAYLOAD_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // VAR_SVC on DIE1
    {
        .ap_base_address = VAR_SVC_DDR_DIE_1_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_VAR_SVC_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_VAR_SVC_END_ADDR,
        .attribute = {ATU_BUS_ATTR_S},
    },
    // Crash dump status header on DIE1
    {
        .ap_base_address = CRASH_DUMP_DDR_HEADER_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_HEADER_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_HEADER_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // Crash dump on DIE1
    {
        .ap_base_address = CRASH_DUMP_DDR_DIE_1_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CRASH_DUMP_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    /**
     * For the D2D send frames, we need to map the send frame on the other die.
     * RMSS HAS section 25.1.1.
     */
    // SCP<->SCP Send Frame
    {
        .ap_base_address = AP_TOP_D0_SCP1R2SCP_MHU_SND_NS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ICC_MHU_SCP2SCP_SEND_FRAME_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ICC_MHU_SCP2SCP_SEND_FRAME_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // MCP<->MCP Send Frame
    {
        .ap_base_address = AP_TOP_D0_MCP3R2MCP_MHU_SND_NS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_ICC_MHU_MCP2MCP_SEND_FRAME_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_ICC_MHU_MCP2MCP_SEND_FRAME_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // IOSS region on DIE0
    {
        .ap_base_address = AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IOSS_D0_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IOSS_D0_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // IOSS region on DIE1
    {
        .ap_base_address = AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_IOSS_D1_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_IOSS_D1_END_ADDR,
        .attribute = {ATU_BUS_ATTR_ROOT},
    },
    // CHBCR for CXL
    {
        .ap_base_address = CHBCR_RESERVATION_BASE,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CHBCR_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CHBCR_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // AP GIC region for RAS notification
    {
        .ap_base_address = AP_TOP_D0_GIC_ADDRESS,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_GIC_GICD_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_GIC_GICD_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    // Cstate timestamps region
    {
        .ap_base_address = AP_CSTATE_BUFFER_DDR_DIE_AP_BASE_ADDR,
        .mscp_start_address = MSCP_ATU_AP_WINDOW_CSTATE_TIMESTAMP_DIE_BASE_ADDR,
        .mscp_end_address = MSCP_ATU_AP_WINDOW_CSTATE_TIMESTAMP_DIE_END_ADDR,
        .attribute = {ATU_BUS_ATTR_NS},
    },
    {0},
};

atu_map_entry_t atu_map_accel_die1[NUM_VALID_ACCEL_ID] = {
    // SDMSS on DIE1
    [ACCEL_ID_SDM] = ATU_MAPPING_SDMSS_BASE(SOC_D1),
    // CDEDSS on DIE1
    [ACCEL_ID_CDED] = ATU_MAPPING_CDEDSS_BASE(SOC_D1),
};

atu_map_entry_t atu_map_ap_accel_cfg_die0[NUM_VALID_ACCEL_ID] = {
    // AP CDED PF SPACE on DIE0
    [ACCEL_ID_SDM] = ATU_MAPPING_AP_SDM_MMIO_BASE(SOC_D0),
    // AP CDED PF SPACE on DIE0
    [ACCEL_ID_CDED] = ATU_MAPPING_AP_CDED_MMIO_BASE(SOC_D0),
};

atu_map_entry_t atu_map_ap_accel_cfg_die1[NUM_VALID_ACCEL_ID] = {
    // AP CDED PF SPACE on DIE1
    [ACCEL_ID_SDM] = ATU_MAPPING_AP_SDM_MMIO_BASE(SOC_D1),
    // AP CDED PF SPACE on DIE1
    [ACCEL_ID_CDED] = ATU_MAPPING_AP_CDED_MMIO_BASE(SOC_D1),
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

void accel_atu_config(ACCEL_ID accel_id)
{
    uint8_t die_num = (uint8_t)idsw_get_die_id();
    atu_map_entry_t* atu_accel_map = NULL;

    BUG_ASSERT_PARAM(accel_id < NUM_VALID_ACCEL_ID, accel_id, NUM_VALID_ACCEL_ID);

    if (die_num == 0)
    {
        atu_accel_map = atu_map_accel_die0;
    }
    else if (die_num == 1)
    {
        atu_accel_map = atu_map_accel_die1;
    }
    else
    {
        BUG_ASSERT_PARAM(false, die_num, 0);
        return;
    }

    // Initialize SDM/CDED ATU
    int status = atu_map(ATU_ID_MSCP, &atu_accel_map[accel_id]);
    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, SILIBS_SUCCESS);
}

int accel_atu_pf_ap_config(ACCEL_ID accel_id, atu_api_type_t operation)
{
    uint8_t die_num = (uint8_t)idsw_get_die_id();
    atu_map_entry_t* atu_accel_map = NULL;
    int status = SILIBS_E_PARAM;

    BUG_ASSERT_PARAM(accel_id < NUM_VALID_ACCEL_ID, accel_id, NUM_VALID_ACCEL_ID);

    if (die_num == 0)
    {
        atu_accel_map = atu_map_ap_accel_cfg_die0;
    }
    else if (die_num == 1)
    {
        atu_accel_map = atu_map_ap_accel_cfg_die1;
    }
    else
    {
        BUG_ASSERT_PARAM(false, die_num, 0);
        return status;
    }

    switch (operation)
    {
    case ATU_IO_REQUEST_MAP_SYNC:
        // Initialize SDM/CDED ATU PF CFG
        status = atu_map(ATU_ID_MSCP, &atu_accel_map[accel_id]);
        break;

    case ATU_IO_REQUEST_UNMAP_SYNC:
        // DeInitialize SDM/CDED ATU PF CFG
        status = atu_unmap(ATU_ID_MSCP, &atu_accel_map[accel_id]);
        break;

    default:
        break;
    }

    BUG_ASSERT_PARAM(status == SILIBS_SUCCESS, status, SILIBS_SUCCESS);
    return status;
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

uint32_t atu_svc_accel_ap_pf_cfg_atu_addr(ACCEL_ID accel_id)
{
    atu_map_entry_t* atu_accel_map;
    uint8_t die_num = (uint8_t)idsw_get_die_id();

    if (accel_id >= NUM_VALID_ACCEL_ID)
    {
        FPFW_RUNTIME_ASSERT(false);
        return 0;
    }

    if (die_num == 0)
    {
        atu_accel_map = atu_map_ap_accel_cfg_die0;
    }
    else if (die_num == 1)
    {
        atu_accel_map = atu_map_ap_accel_cfg_die1;
    }
    else
    {
        FPFW_RUNTIME_ASSERT(false);
        return 0;
    }

    return atu_accel_map[accel_id].mscp_start_address;
}

uint32_t atu_svc_accel_atu_addr(ACCEL_ID accel_id)
{
    atu_map_entry_t* atu_accel_map;
    uint8_t die_num = (uint8_t)idsw_get_die_id();

    if (accel_id >= NUM_VALID_ACCEL_ID)
    {
        FPFW_RUNTIME_ASSERT(false);
        return 0;
    }

    if (die_num == 0)
    {
        atu_accel_map = atu_map_accel_die0;
    }
    else if (die_num == 1)
    {
        atu_accel_map = atu_map_accel_die1;
    }
    else
    {
        FPFW_RUNTIME_ASSERT(false);
        return 0;
    }

    return atu_accel_map[accel_id].mscp_start_address;
}
