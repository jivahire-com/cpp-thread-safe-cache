//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core.c
 * Implements the APcore driver interface.
 */

/*------------- Includes -----------------*/
#include "ap_core.h"

#include "accelerator_ip.h"
#include "ap_core_i.h"
#include "ap_core_init.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <corebits.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send, fpfw_icc_base...
#include <fuse_init.h>
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#include <inttypes.h>
#include <kng_error.h>
#define __NO_CSR_TYPEDEFS__
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <pik_clock_lib.h>
#include <sds_api.h>
#include <sds_configuration.h>
#include <sds_init.h>
#include <shared_sds_def.h>
#include <silibs_platform.h>
#include <startup_shutdown_ssi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static ap_core_service_context_t s_ap_core_ctx = {0};
static fpfw_icc_base_ctx_t* s_icc_base_ctx = NULL;

/*------------- Functions ----------------*/
static void ap_core_die_config_handover()
{
    shared_scp_exp_csr_die_config die_config_val = {
        .as_uint32 = MMIO_READ32(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS + SCP_EXP_CSR_DIE_CONFIG_ADDRESS)};

    int32_t result = sds_block_creation(SDS_DIE_CONFIG_STRUCT_ID, SDS_DIE_CONFIG_SIZE, PLATFORM_SDS_REGION_ARSM_DIE0);
    BUG_ASSERT(result == KNG_SUCCESS);

    result = sds_block_write(SDS_DIE_CONFIG_STRUCT_ID, &die_config_val, SDS_DIE_CONFIG_SIZE);
    BUG_ASSERT(result == KNG_SUCCESS);

    write_fuse_info_to_ap();
}

// dispatcher function to handle set of rvbaraddr
static void ap_core_dispatch_set_rvbaraddr(pap_core_asynchronous_request_t p_request)
{
    APCORE_LOG_INFO("Set RVBARADDR, address %" PRIx32 "%08" PRIx32,
                    (uint32_t)((p_request->data.rvbaraddr) >> 32),
                    (uint32_t)p_request->data.rvbaraddr);

    // call utility function to set all rvbar addresses
    ap_core_util_set_all_rvbaraddr(&s_ap_core_ctx, p_request->data.rvbaraddr);
    // complete immediately, since handling is done
    // TODO: revisit for multi-die
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1869647
    DfwkAsyncRequestComplete(&p_request->header);
}

// dispatcher function to handle core power on
static void ap_core_dispatch_core_power_on(pap_core_asynchronous_request_t p_request)
{
    APCORE_LOG_INFO("Core power on, core ID %u", (unsigned)p_request->data.core_id);

    // power on core
    ap_core_ppu_core_set_power_state(&s_ap_core_ctx, p_request->data.core_id, true, DEFAULT_POWER_TRANSITION_TIMEOUT_MS);
    // complete p_request; for local cores, there's nothing to wait on
    // TODO: revisit for multi-die
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1869647
    DfwkAsyncRequestComplete(&p_request->header);
}

// dispatcher function to handle core power off
static void ap_core_dispatch_core_power_off(pap_core_asynchronous_request_t p_request)
{
    APCORE_LOG_INFO("Core power off, core ID %u", (unsigned)p_request->data.core_id);

    // power on core
    ap_core_ppu_core_set_power_state(&s_ap_core_ctx, p_request->data.core_id, false, DEFAULT_POWER_TRANSITION_TIMEOUT_MS);
    // complete p_request; for local cores, there's nothing to wait on
    // TODO: revisit for multi-die
    //       https://azurecsi.visualstudio.com/Dev/_workitems/edit/1869647
    DfwkAsyncRequestComplete(&p_request->header);
}

// dispatcher function to handle boot stage for cluster init
static void ap_core_ssi_start_cluster_init(pssi_startup_notification_request_t p_request)
{
    // turn on cluster PPUs
    APCORE_LOG_INFO("Cluster power on");
    ap_core_ppu_clusters_on(&s_ap_core_ctx, DEFAULT_POWER_TRANSITION_TIMEOUT_MS);
    // for now, PPU is synchronous
    DfwkAsyncRequestComplete(&p_request->header);
}

// dispatcher function to handle boot stage for primary AP core boot
static void ap_core_ssi_start_primary_ap_core_boot(pssi_startup_notification_request_t p_request, ssi_startup_type_t boot_type)
{
    unsigned int boot_core = ap_core_util_boot_core(&s_ap_core_ctx);
    APCORE_LOG_INFO("Primary AP core power on (%d)", boot_core);

    ap_core_die_config_handover();
    ap_core_util_set_rvbaraddr(&s_ap_core_ctx, boot_core, s_ap_core_ctx.p_config->boot_core_rvbaraddr);

    if (boot_type != COLD_BOOT)
    {
        ap_core_ppu_clusters_on_if_needed(&s_ap_core_ctx, DEFAULT_POWER_TRANSITION_TIMEOUT_MS);
    }

    ap_core_ppu_core_set_power_state(&s_ap_core_ctx, boot_core, true, DEFAULT_POWER_TRANSITION_TIMEOUT_MS);
    // for now, PPU is synchronous
    DfwkAsyncRequestComplete(&p_request->header);
}

// dispatcher function to handle shutdown scenario
static void ap_core_ssi_shutdown_quiesce(pssi_shutdown_notification_request_t p_request)
{
    APCORE_LOG_TRACE("SSI shutdown, shutdown type %d", p_request->shutdown_type);

    if (p_request->shutdown_type != MSCP_SUBSYS_RESET)
    {
        // turn off all cores
        ap_core_ppu_cores_off(&s_ap_core_ctx, DEFAULT_POWER_TRANSITION_TIMEOUT_MS);

        // turn off clusters
        ap_core_ppu_clusters_off(&s_ap_core_ctx, DEFAULT_POWER_TRANSITION_TIMEOUT_MS);
    }

    DfwkAsyncRequestComplete(&p_request->header);
}

// asynchronous dispatch function
void ap_core_dispatch(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    FPFW_UNUSED(p_context);
    s_ap_core_ctx.outstanding_request = (pap_core_asynchronous_request_t)p_request;

    switch (p_request->RequestType)
    {

        /* these are service-specific requests */

    case APCORE_SET_RVBARADDR_ASYNC:
        ap_core_dispatch_set_rvbaraddr((pap_core_asynchronous_request_t)p_request);
        break;
    case APCORE_CORE_POWER_ON_ASYNC:
        ap_core_dispatch_core_power_on((pap_core_asynchronous_request_t)p_request);
        break;
    case APCORE_CORE_POWER_OFF_ASYNC:
        ap_core_dispatch_core_power_off((pap_core_asynchronous_request_t)p_request);
        break;

        /* these are requests used to initiate start phases or a shutdown */

    case SSI_STARTUP_STAGE_START_ASYNC: {
        pssi_startup_notification_request_t ssi_request = (pssi_startup_notification_request_t)p_request;
        switch (ssi_request->stage)
        {
        case STARTUP_CLUSTER_CORE_INIT:
            ap_core_ssi_start_cluster_init(ssi_request);
            break;
        case STARTUP_PCIE_PHY_LOAD:
            if (system_info_is_hsp_present() && ssi_request->boot_type == COLD_BOOT)
            {
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_PCIE_PHY);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }
            break;
        case STARTUP_PRIMARY_AP_CORE_BOOT:
            if (s_ap_core_ctx.p_config->primary_boot_die)
            {
                // Turn on primary AP core only on primary boot die
                ap_core_ssi_start_primary_ap_core_boot(ssi_request, ssi_request->boot_type);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_BL31_LOAD:
            if (s_ap_core_ctx.p_config->primary_boot_die && system_info_is_hsp_present() && ssi_request->boot_type == COLD_BOOT)
            {
                // Primary boot die && HSP present then request TFA firmware load (BL31)
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_BL31);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_STMM_LOAD:
            if (s_ap_core_ctx.p_config->primary_boot_die && system_info_is_hsp_present() && ssi_request->boot_type == COLD_BOOT)
            {
                // Primary boot die && HSP present then request STMM firmware load
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_STMM);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_BL33_LOAD:
            if (s_ap_core_ctx.p_config->primary_boot_die && system_info_is_hsp_present() && ssi_request->boot_type == COLD_BOOT)
            {
                // Primary boot die && HSP present then request BL33 firmware load
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_BL33);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_HAFNIUM_LOAD:
            if (s_ap_core_ctx.p_config->primary_boot_die && system_info_is_hsp_present() && ssi_request->boot_type == COLD_BOOT)
            {
                // Primary boot die && HSP present then request Hafnium firmware load
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_HAFNIUM);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_RMM_LOAD:
            if (s_ap_core_ctx.p_config->primary_boot_die && system_info_is_hsp_present() && ssi_request->boot_type == COLD_BOOT)
            {
                // Primary boot die && HSP present then request RMM firmware load
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_RMM);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_RP_EXE_LOAD:
            if (s_ap_core_ctx.p_config->primary_boot_die && system_info_is_hsp_present() && ssi_request->boot_type == COLD_BOOT)
            {
                // Primary boot die && HSP present then request RP_EXE firmware load
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_RP_EXE);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_RP_DATA_LOAD:
            if (s_ap_core_ctx.p_config->primary_boot_die && system_info_is_hsp_present() && ssi_request->boot_type == COLD_BOOT)
            {
                // Primary boot die && HSP present then request RP_DATA firmware load
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_RP_DATA);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_MCP_LOAD:
            if (system_info_is_hsp_present())
            {
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_MCP);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_SDM_ITCM_LOAD:
            if (system_info_is_hsp_present() && !accel_is_isolation_enabled(ACCEL_ID_SDM))
            {
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_SDM_ITCM);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_SDM_DTCM_LOAD:
            if (system_info_is_hsp_present() && !accel_is_isolation_enabled(ACCEL_ID_SDM))
            {
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_SDM_DTCM);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_CDED_ITCM_LOAD:
            if (system_info_is_hsp_present() && !accel_is_isolation_enabled(ACCEL_ID_CDED))
            {
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_CDED_ITCM);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_CDED_DTCM_LOAD:
            if (system_info_is_hsp_present() && !accel_is_isolation_enabled(ACCEL_ID_CDED))
            {
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_CDED_DTCM);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        case STARTUP_KMP_LOAD:
            if (system_info_is_hsp_present() && !accel_is_isolation_enabled(ACCEL_ID_CDED))
            {
                ap_core_request_load_ap_fw(s_icc_base_ctx, AP_FW_ID_KMP);
            }
            else
            {
                DfwkAsyncRequestComplete(p_request);
            }

            break;
        default:
            // nothing to do for other types.
            // complete p_request
            DfwkAsyncRequestComplete(p_request);
            break;
        }
    }
    break;
    case SSI_STARTUP_STAGE_COMPLETE_ASYNC:
        // complete immediately, since nothing to do
        DfwkAsyncRequestComplete(p_request);
        break;
    case SSI_SHUTDOWN_QUIESCE_ASYNC: {
        pssi_shutdown_notification_request_t ssi_request = (pssi_shutdown_notification_request_t)p_request;
        ap_core_ssi_shutdown_quiesce(ssi_request);
    }
    break;
    default:
        FPFW_RUNTIME_ASSERT((false));
        break;
    }
}

// synchronous dispatch function
int32_t ap_core_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER p_request)
{
    switch (p_request->RequestType)
    {

    // service doesn't currently support any synchronous requests
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
    return DFWK_SUCCESS; // return success
}

void ap_core_interface_init(pap_core_service_t p_device, pap_core_interface_t p_interface)
{
    FPFW_RUNTIME_ASSERT(p_device != NULL);
    FPFW_RUNTIME_ASSERT(p_interface != NULL);

    DfwkInterfaceInitialize(&p_interface->header, &p_device->header, &p_device->default_queue, ap_core_dispatch_sync);
    p_interface->p_device = p_device;
}

void ap_core_init(pap_core_service_t p_device,
                  PDFWK_SCHEDULE p_schedule,
                  fpfw_icc_base_ctx_t* icc_base_ctx,
                  const ap_core_service_config_t* p_config)
{
    FPFW_RUNTIME_ASSERT(p_device != NULL);
    FPFW_RUNTIME_ASSERT(p_schedule != NULL);
    FPFW_RUNTIME_ASSERT(p_config != NULL);

    APCORE_LOG_INFO("Init");
    DfwkDeviceInitialize(&p_device->header, p_schedule);
    DfwkQueueInitialize(&p_device->default_queue, &p_device->header, ap_core_dispatch, &p_device->header, DfwkQueueType_SerializedDispatch);

    // store off device config
    s_ap_core_ctx.p_config = p_config;
    s_icc_base_ctx = icc_base_ctx;

    // read fuses to get enabled cores
    ap_core_util_get_fuse_enabled_cores(&s_ap_core_ctx.enabled_cores);
    // limit enabled cores to those that are actually present
    corebits_and(&s_ap_core_ctx.enabled_cores, p_config->platform_cores_in_die);

    // initialize pik cluster remap base
    pik_clock_set_cluster_remapped_base(p_config->cluster_pex_base);

    ap_core_ppu_init(&s_ap_core_ctx);
}

pap_core_asynchronous_request_t ap_core_get_outstanding_request()
{
    return s_ap_core_ctx.outstanding_request;
}
