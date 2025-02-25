//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_injection.c
 * Implements error injection related functionality.
 */

/*------------- Includes -----------------*/
#include <bug_check.h>
#include <health_monitor_i.h>
#include <icc_platform_defines.h>
#include <idhw.h>
#include <idsw.h>

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

    hm_prepare_error_injection_listener(hm_config->icc_ctx[HM_INTERCORE_SCP]);
    hm_inject_error_local();
}

void hm_prepare_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    static rmss_d2d_mailbox_msg einj_relay_msg = {0};
    static fpfw_icc_base_recv_req_t scp_recv_params = {
        .payload_buffer = &einj_relay_msg,
        .buffer_size = sizeof(einj_relay_msg),
        .recv_cmd_code = RMSS_D2D_MAILBOX_MSG_ERROR_INJECTION_REQ,
        .cb = hm_inject_error_recv_cb,
        .cb_ctx = NULL,
    };

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &scp_recv_params);
    BUG_ASSERT_PARAM(status == FPFW_ICC_BASE_STATUS_SUCCESS, status, icc_ctx);
}

acpi_einj_cmd_status_t hm_inject_error(void)
{
    acpi_einj_cmd_status_t status = ACPI_EINJ_SUCCESS;

    hm_config_t* hm_config = get_hm_config();
    volatile ras_einj_info_t* einj_payload = (ras_einj_info_t*)hm_config->mscp_error_injection_addr_base;

    if (einj_payload->component_instance <= 1)
    {
        if (einj_payload->component_instance != idsw_get_die_id())
        {
            HM_LOG_INFO("Send EINJ req to remote");

            static rmss_d2d_mailbox_msg einj_relay_msg = {0};
            einj_relay_msg.as_uint32[0] =
                SET_RMSS_D2D_MAILBOX_HEADER_ASUNIT32(RMSS_D2D_MAILBOX_MSG_ERROR_INJECTION_REQ, 0, 0);

            static fpfw_icc_base_send_req_t scp_send_params = {
                .payload_buffer = &einj_relay_msg,
                .buffer_size = sizeof(einj_relay_msg),
                .cb = hm_inject_error_send_cb,
                .cb_ctx = NULL,
            };

            fpfw_status_t icc_status = fpfw_icc_base_send(hm_config->icc_ctx[HM_INTERCORE_SCP], &scp_send_params);
            if (icc_status != FPFW_ICC_BASE_STATUS_SUCCESS)
            {
                HM_LOG_CRIT("Failed to send EINJ req to remote");
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
        HM_LOG_CRIT("Invalid instance in EINJ payload(%d)", einj_payload->component_instance);
        status = ACPI_EINJ_INVALID_ACCESS;
    }

    return status;
}

acpi_einj_cmd_status_t hm_inject_error_local(void)
{
    ras_einj_info_t einj_payload = {0};
    hm_config_t* hm_config = get_hm_config();

    wait_for_semaphore(hm_config->semaphore_id, hm_config->semaphore_key);
    for (size_t i = 0; i < sizeof(ras_einj_info_t); i++)
    {
        ((uint8_t*)&einj_payload)[i] = ((volatile uint8_t*)hm_config->mscp_error_injection_addr_base)[i];
    }
    release_semaphore(hm_config->semaphore_id);

    if (einj_payload.version != (uint32_t)ERROR_INJECTION_PAYLOAD_VERSION)
    {
        HM_LOG_CRIT("Invalid EINJ payload version(%d)", (int)einj_payload.version);
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
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // 4. Check if error domain is enabled
    hm_error_domain_info_t* err_domain = hm_get_registered_error_domain(target_error_domain_idx);

    if (err_domain != NULL)
    {
        HM_LOG_INFO("Invoke EINJ handler for (%d)", (int)target_error_domain_idx);
    }
    else
    {
        HM_LOG_INFO("Err domain(%d) not activated", (int)target_error_domain_idx);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    return err_domain->injection_cb(&einj_payload, err_domain->err_inject_ctx);
}
