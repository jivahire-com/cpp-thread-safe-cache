//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
/**
 * @file ib_telm_hsp_init.c
 * Sends the HSP a notification that the AP window is ready for in-band telemetry.
 */

/*------------- Includes -----------------*/

#include <FpFwUtils.h>
#include <boot_status.h>
#include <fpfw_icc_base.h>
#include <fpfw_init.h>
#include <hsp_firmware_headers.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <in_band_telemetry_ddr.h>
#include <stddef.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(ib_telm_hsp, FPFW_INIT_DEPENDENCIES("icc_hspmbx", "hw_ver", "ddr", "boot_stat"))
{
    // Send the hsp where in DDR it can store logs now that DDR is initialized
    uint64_t hsp_ddr_mem_ap_address = IB_TLM_DDR_ATU_AP_ADDR_TRACE_HSP_BASE_ADDR(idsw_get_die_id());

    kng_hsp_mailbox_msg msg;
    msg.header.cmd = HSP_MAILBOX_CMD_TELEMETRY_DDR_ADDR_NOTIFY;
    msg.telm_ddr_addr_notify.ddr_reserved_addr_low = (uint32_t)hsp_ddr_mem_ap_address;
    msg.telm_ddr_addr_notify.ddr_reserved_addr_high = (uint32_t)(hsp_ddr_mem_ap_address >> 32);

    // Fire and Forget
    fpfw_icc_base_ctx_t* icc_ctx = fpfw_init_get_handle("icc_hspmbx");
    fpfw_status_t icc_base_status = fpfw_icc_base_send_sync(icc_ctx, &msg, sizeof(kng_hsp_mailbox_msg));

    if (FPFW_STATUS_FAILED(icc_base_status))
    {
        return (fpfw_init_result_t){icc_base_status, NULL};
    }

    boot_status_req_t boot_status_req = {0};
    boot_status_notify_extd(
        &boot_status_req,
        (idsw_get_cpu_type() == CPU_SCP) ? MSCP_BOOT_STATUS_CODE_SCP_IB_TELM_INIT_END : MSCP_BOOT_STATUS_CODE_MCP_IB_TELM_INIT_END,
        GEN_BOOT_STATUS_EX_GENERIC_CODE((idsw_get_cpu_type() == CPU_SCP) ? COMPONENT_GROUP_SCP : COMPONENT_GROUP_MCP,
                                        MSCP_GENERIC,
                                        (idsw_get_die_id() == DIE_0)
                                            ? ((idsw_get_cpu_type() == CPU_SCP) ? SCP_PRIMARY : MCP_PRIMARY)
                                            : ((idsw_get_cpu_type() == CPU_SCP) ? SCP_SECONDARY : MCP_SECONDARY)));

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
