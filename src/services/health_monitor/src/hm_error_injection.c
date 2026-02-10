//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_injection.c
 * Implements error injection related functionality.
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <fpfw_icc_base.h>
#include <health_monitor_events.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <icc_platform_defines.h>
#include <idhw.h>
#include <idsw.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void hm_inject_error_req_cb(void* context, size_t output_size_bytes, fpfw_status_t status);
acpi_einj_cmd_status_t hm_inject_error_local(void);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void hm_inject_error_send_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

void hm_inject_error_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(output_size_bytes);
    FPFW_UNUSED(status);

    hm_config_t* hm_config = get_hm_config();

    hm_prepare_error_injection_listener(hm_config->icc_ctx[HM_INTERCORE_LOCAL]);
    hm_inject_error_local();
}

void hm_prepare_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    static icc_mhu_header_t einj_relay_payload;

    static fpfw_icc_base_recv_req_t einj_routing_recv_params;
    einj_routing_recv_params.recv_cmd_code = ICC_HM_ERROR_INJECTION_ROUTING_REQ;
    einj_routing_recv_params.payload_buffer = &einj_relay_payload;
    einj_routing_recv_params.buffer_size = sizeof(einj_relay_payload);
    einj_routing_recv_params.cb = hm_inject_error_recv_cb;
    einj_routing_recv_params.cb_ctx = NULL;

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &einj_routing_recv_params);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, icc_ctx);
}

acpi_einj_cmd_status_t hm_inject_error(void)
{
    uint32_t injection_target = 0;
    acpi_einj_cmd_status_t status = ACPI_EINJ_SUCCESS;

    if (system_info_get_mission_mode() == true)
    {
        HM_LOG_CRIT("Error injection not allowed in mission mode");
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (hm_allow_ras_reporting() == false)
    {
        HM_LOG_CRIT("RAS disabled");
        return ACPI_EINJ_INVALID_ACCESS;
    }

    hm_config_t* hm_config = get_hm_config();

    hm_map_error_injection_payload();
    volatile ras_einj_info_t* einj_payload = (ras_einj_info_t*)hm_config->mscp_error_injection_addr_base;
    injection_target = einj_payload->component_instance;
    hm_unmap_error_injection_payload();

    if (idhw_is_single_die_boot_en() == true)
    {
        if (injection_target != idsw_get_die_id())
        {
            HM_LOG_CRIT("Invalid DIE_ID(%d)", injection_target);
            HM_ET_ERROR_PARAM(HM_ET_TYPE_INJECTION_INVALID_ACCESS, injection_target);
            return ACPI_EINJ_INVALID_ACCESS;
        }
    }

    if (injection_target <= 1)
    {
        if (injection_target != idsw_get_die_id())
        {
            HM_LOG_INFO("error injection request was made for remote core, re-route");

            static icc_mhu_header_t einj_relay_msg = {0};
            einj_relay_msg.msg_header.command = ICC_HM_ERROR_INJECTION_ROUTING_REQ;

            static fpfw_icc_base_send_req_t einj_routing_send_params = {.payload_buffer = &einj_relay_msg,
                                                                        .buffer_size = sizeof(einj_relay_msg),
                                                                        .cb = hm_inject_error_send_cb,
                                                                        .cb_ctx = NULL};

            fpfw_status_t icc_status =
                fpfw_icc_base_send(hm_config->icc_ctx[HM_INTERCORE_LOCAL], &einj_routing_send_params);
            if (icc_status != FPFW_ICC_BASE_STATUS_SUCCESS)
            {
                HM_LOG_CRIT("failed to route injection request to remote");
                HM_ET_ERROR_PARAM(HM_ET_TYPE_INJECTION_ICC_TRANSFER, icc_status);
                status = ACPI_EINJ_UNKNOWN_FAILURE;
            }
        }
        else
        {
            status = hm_inject_error_local();
        }
    }
    else
    {
        HM_LOG_CRIT("Invalid DIE_ID(%d)", einj_payload->component_instance);
        HM_ET_ERROR_PARAM(HM_ET_TYPE_INJECTION_INVALID_ACCESS, einj_payload->component_instance);
        status = ACPI_EINJ_INVALID_ACCESS;
    }

    return status;
}

acpi_einj_cmd_status_t hm_inject_error_local(void)
{
    ras_einj_info_t einj_payload = {0};
    hm_config_t* hm_config = get_hm_config();

    hm_map_error_injection_payload();
    wait_for_semaphore(hm_config->semaphore_id, hm_config->semaphore_key);
    for (size_t i = 0; i < sizeof(ras_einj_info_t); i++)
    {
        ((uint8_t*)&einj_payload)[i] = ((volatile uint8_t*)hm_config->mscp_error_injection_addr_base)[i];
    }
    release_semaphore(hm_config->semaphore_id);
    hm_unmap_error_injection_payload();

    if (einj_payload.version != (uint32_t)ERROR_INJECTION_PAYLOAD_VERSION)
    {
        HM_LOG_CRIT("Invalid EINJ payload version(%d)", (int)einj_payload.version);
        HM_ET_ERROR_PARAM(HM_ET_TYPE_INJECTION_INVALID_ACCESS, (int)einj_payload.version);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    acpi_error_domain_t target_error_domain_idx = ACPI_ERROR_DOMAIN_INVALID;

    if ((acpi_error_domain_t)einj_payload.component_group < ACPI_ERROR_DOMAIN_COUNT)
    {
        target_error_domain_idx = (acpi_error_domain_t)einj_payload.component_group;
    }
    else
    {
        HM_LOG_CRIT("Invalid err domain in EINJ payload(%d)", einj_payload.component_group);
        HM_ET_ERROR_PARAM(HM_ET_TYPE_INJECTION_INVALID_ACCESS, einj_payload.component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // 4. Check if error domain is enabled
    hm_error_domain_info_t* err_domain = hm_get_registered_error_domain(target_error_domain_idx);

    if (err_domain != NULL)
    {
        HM_LOG_INFO("Invoke %s EINJ handler", get_error_domain_name(target_error_domain_idx));
    }
    else
    {
        HM_LOG_INFO("%s error domain not activated", get_error_domain_name(target_error_domain_idx));
        HM_ET_ERROR_PARAM(HM_ET_TYPE_INJECTION_INVALID_ACCESS, target_error_domain_idx);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    return err_domain->injection_cb(&einj_payload, err_domain->err_inject_ctx);
}
