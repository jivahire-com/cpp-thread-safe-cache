//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file hm_error_domain_accel.c
 * Implements accelerator error domain related functions.
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <accelip_id.h>
#include <atu_init.h>
#include <bug_check.h>
#include <cper.h>
#include <fpfw_icc_base.h>
#include <health_monitor.h>
#include <health_monitor_i.h>
#include <health_monitor_icc.h>
#include <icc_platform_defines.h>
#include <mscp_exp_rmss_memory_map.h>
#include <sdm_ext_cfg_regs.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

typedef enum
{
    TFR_STATE_IDLE,
    TFR_STATE_TX_CB_PEND,
    TFR_STATE_RX_CB_COMP,
    TFR_STATE_RX_DONE,
    TFR_STATE_TX_ERR,
    TFR_STATE_RX_REG_ERR,
} e_hm_accel_tfr_state_t;

static hm_accel_cper_info_t accel_cper_info[NUM_VALID_ACCEL_ID];
static hm_accel_error_domain_register_payload_t hm_icc_accel_err_register_payload[NUM_VALID_ACCEL_ID];
static fpfw_icc_base_recv_req_t hm_icc_accel_err_register_recv_req[NUM_VALID_ACCEL_ID];
static hm_accel_error_injection_payload_t accel_err_injection_payload[NUM_VALID_ACCEL_ID];
static fpfw_icc_base_send_req_t hm_icc_accel_err_injection_req[NUM_VALID_ACCEL_ID];

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

static void hm_accel_tx_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

static void hm_accel_ack_tx_cb(void* context, fpfw_status_t status)
{
    FPFW_UNUSED(context);
    FPFW_UNUSED(status);
}

static bool hm_accel_error_record_submit_listener(fpfw_icc_base_ctx_t* icc_ctx, hm_accel_cper_info_t* cper_info)
{
    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &cper_info->hm_icc_sdm_err_submit_req);
    return status == FPFW_ICC_BASE_STATUS_SUCCESS ? true : false;
}

static fpfw_status_t hm_accel_error_record_ack_tx(fpfw_icc_base_ctx_t* icc_ctx, hm_accel_msg_t* accel_mesg, ACCEL_ID accel_id)
{
    static hm_accel_msg_ack_t hm_accel_ack_payload[NUM_VALID_ACCEL_ID] = {0};

    hm_accel_ack_payload[accel_id].header.cmd = ICC_HM_TX_DONE_ACK_ACCEL(accel_id);
    hm_accel_ack_payload[accel_id].cper_buffer_offset = accel_mesg->dtcm_mem_offset;

    static fpfw_icc_base_send_req_t hm_icc_sdm_cper_acq_req[NUM_VALID_ACCEL_ID] = {0};
    hm_icc_sdm_cper_acq_req[accel_id].payload_buffer = &hm_accel_ack_payload[accel_id];
    hm_icc_sdm_cper_acq_req[accel_id].buffer_size = sizeof(hm_accel_msg_ack_t);
    hm_icc_sdm_cper_acq_req[accel_id].cb = hm_accel_ack_tx_cb;
    hm_icc_sdm_cper_acq_req[accel_id].cb_ctx = NULL;

    return fpfw_icc_base_send(icc_ctx, &hm_icc_sdm_cper_acq_req[accel_id]);
}

static void hm_accel_error_record_submit_listener_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    hm_accel_cper_info_t* curr_cper_info = (hm_accel_cper_info_t*)context;
    hm_accel_msg_t* payload = (hm_accel_msg_t*)&curr_cper_info->msg_payload;
    uint32_t accel_dtcm_base_addr =
        (SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS + atu_svc_accel_atu_addr(curr_cper_info->accel_id));
    uint8_t* curr_cper_buffer_ptr = (uint8_t*)(accel_dtcm_base_addr + payload->dtcm_mem_offset);
    hm_intercore_type_t core_type = (curr_cper_info->accel_id == ACCEL_ID_SDM) ? HM_INTERCORE_SDM : HM_INTERCORE_CDED;

    if (status != FPFW_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("%s CPER ICC get failed(%d)", curr_cper_info->accel_id == ACCEL_ID_SDM ? "SDM" : "CDED", (int)status);
        return;
    }

    if (curr_cper_info == NULL || payload->header.cmd != ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(curr_cper_info->accel_id))
    {
        HM_LOG_CRIT("Invalid %s CPER payload(%p)", curr_cper_info->accel_id == ACCEL_ID_SDM ? "SDM" : "CDED", curr_cper_info);
        return;
    }

    // memcpy the ICC buffer to global cper buffer
    memcpy((void*)&curr_cper_info->accel_err_payload, (void*)curr_cper_buffer_ptr, sizeof(acpi_err_sec_accel_vendor_t));
    curr_cper_info->err_severity = (uint32_t)(*((curr_cper_buffer_ptr) + sizeof(acpi_err_sec_accel_vendor_t)));

    acpi_cper_section_t cper_section;
    cper_section.sec_sdm = curr_cper_info->accel_err_payload;

    hm_submit_cper(curr_cper_info->error_domain_index, curr_cper_info->err_severity, &cper_section, sizeof(acpi_err_sec_accel_vendor_t));

    hm_config_t* hm_config = get_hm_config();
    hm_accel_error_record_submit_listener(hm_config->icc_ctx[core_type], curr_cper_info);
    hm_accel_error_record_ack_tx(hm_config->icc_ctx[core_type], payload, curr_cper_info->accel_id);
}

static acpi_einj_cmd_status_t hm_accel_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx)
{
    acpi_einj_cmd_status_t result = ACPI_EINJ_SUCCESS;
    ACCEL_ID accel_id = (ACCEL_ID)ctx;
    hm_intercore_type_t core_type = (accel_id == ACCEL_ID_SDM) ? HM_INTERCORE_SDM : HM_INTERCORE_CDED;

    accel_err_injection_payload[accel_id].header.cmd = ICC_HM_ERROR_INJECTION_ACCEL(accel_id);

    hm_icc_accel_err_injection_req[accel_id].payload_buffer = &accel_err_injection_payload[accel_id];
    hm_icc_accel_err_injection_req[accel_id].buffer_size = sizeof(hm_accel_error_injection_payload_t);
    hm_icc_accel_err_injection_req[accel_id].cb = hm_accel_tx_cb;
    hm_icc_accel_err_injection_req[accel_id].cb_ctx = NULL;

    memcpy(&accel_err_injection_payload[accel_id].error_injection_info, einj_payload, sizeof(ras_einj_info_t));

    fpfw_status_t status =
        fpfw_icc_base_send(get_hm_config()->icc_ctx[core_type], &hm_icc_accel_err_injection_req[accel_id]);

    if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("%s error injection request failed(%d)", accel_id == ACCEL_ID_SDM ? "SDM" : "CDED", (int)status);
        result = ACPI_EINJ_UNKNOWN_FAILURE;
    }
    else
    {
        HM_LOG_INFO("%s error injection request was made", accel_id == ACCEL_ID_SDM ? "SDM" : "CDED");
    }

    return result;
}

static void hm_accel_register_error_domain(hm_accel_error_domain_register_payload_t* hm_err_register_payload,
                                           ACCEL_ID accel_id,
                                           hm_intercore_type_t core_type)
{
    if (hm_err_register_payload == NULL ||
        hm_err_register_payload->header.cmd != ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(accel_id))
    {
        HM_LOG_CRIT("Invalid %s registration payload", accel_id == ACCEL_ID_SDM ? "SDM" : "CDED");
        return;
    }

    hm_register_error_domain(hm_err_register_payload->error_domain_idx,
                             hm_err_register_payload->valid_fru_id ? &hm_err_register_payload->fru_id : NULL,
                             hm_err_register_payload->valid_fru_text ? (char*)hm_err_register_payload->fru_text : NULL,
                             hm_accel_error_injection_cb,
                             (void*)accel_id);

    hm_config_t* hm_config = get_hm_config();
    hm_accel_error_record_submit_listener(hm_config->icc_ctx[core_type], &accel_cper_info[accel_id]);

    // Using the ack message as payload as the mailbox primitives have been
    // updated allowing for smaller packets to be sent
    accel_cper_info[accel_id].msg_payload.header.cmd = ICC_HM_ERROR_DOMAIN_REGISTER_DONE_ACK_ACCEL(accel_id);

    hm_icc_accel_err_injection_req[accel_id].payload_buffer = &accel_cper_info[accel_id].msg_payload;
    hm_icc_accel_err_injection_req[accel_id].buffer_size = sizeof(hm_accel_msg_t);
    hm_icc_accel_err_injection_req[accel_id].cb = hm_accel_tx_cb;
    hm_icc_accel_err_injection_req[accel_id].cb_ctx = NULL;

    fpfw_status_t status = fpfw_icc_base_send(hm_config->icc_ctx[core_type], &hm_icc_accel_err_injection_req[accel_id]);
    if (status != FPFW_ICC_BASE_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("%s registration ack failed (%d)", accel_id == ACCEL_ID_SDM ? "SDM" : "CDED", (int)status);
    }
}

static void hm_sdm_error_domain_register_listener_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    if (status != FPFW_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("SDM registration failed(%d)", (int)status);
        return;
    }

    hm_accel_register_error_domain((hm_accel_error_domain_register_payload_t*)context, ACCEL_ID_SDM, HM_INTERCORE_SDM);
}

static void hm_cded_sdm_error_domain_register_listener_cb(void* context, size_t output_size_bytes, fpfw_status_t status)
{
    FPFW_UNUSED(output_size_bytes);

    if (status != FPFW_STATUS_SUCCESS)
    {
        HM_LOG_CRIT("CDED registration failed(%d)", (int)status);
        return;
    }

    hm_accel_register_error_domain((hm_accel_error_domain_register_payload_t*)context, ACCEL_ID_CDED, HM_INTERCORE_CDED);
}

void hm_prepare_sdm_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    HM_LOG_INFO("Waiting SDM error domain registration");

    // Setup the info for cper submission callback
    accel_cper_info[ACCEL_ID_SDM].error_domain_index = ACPI_ERROR_DOMAIN_SDM;
    accel_cper_info[ACCEL_ID_SDM].hm_icc_sdm_err_submit_req.recv_cmd_code = ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_SDM);
    accel_cper_info[ACCEL_ID_SDM].hm_icc_sdm_err_submit_req.payload_buffer = &accel_cper_info[ACCEL_ID_SDM].msg_payload;
    accel_cper_info[ACCEL_ID_SDM].hm_icc_sdm_err_submit_req.buffer_size = sizeof(hm_accel_msg_t);
    accel_cper_info[ACCEL_ID_SDM].hm_icc_sdm_err_submit_req.cb = hm_accel_error_record_submit_listener_cb;
    accel_cper_info[ACCEL_ID_SDM].hm_icc_sdm_err_submit_req.cb_ctx = &accel_cper_info[ACCEL_ID_SDM];
    accel_cper_info[ACCEL_ID_SDM].accel_id = ACCEL_ID_SDM;

    hm_icc_accel_err_register_recv_req[ACCEL_ID_SDM].recv_cmd_code = ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_SDM);
    hm_icc_accel_err_register_recv_req[ACCEL_ID_SDM].payload_buffer =
        (void*)&hm_icc_accel_err_register_payload[ACCEL_ID_SDM];
    hm_icc_accel_err_register_recv_req[ACCEL_ID_SDM].buffer_size = sizeof(hm_accel_error_domain_register_payload_t);
    hm_icc_accel_err_register_recv_req[ACCEL_ID_SDM].cb = hm_sdm_error_domain_register_listener_cb;
    hm_icc_accel_err_register_recv_req[ACCEL_ID_SDM].cb_ctx = (void*)&hm_icc_accel_err_register_payload[ACCEL_ID_SDM];

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_icc_accel_err_register_recv_req[ACCEL_ID_SDM]);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);
}

void hm_prepare_cded_sdm_listener(fpfw_icc_base_ctx_t* icc_ctx)
{
    HM_LOG_INFO("Waiting CDED error domain registration");

    // Setup the info for cper submission callback
    accel_cper_info[ACCEL_ID_CDED].error_domain_index = ACPI_ERROR_DOMAIN_CDED_SDM;
    accel_cper_info[ACCEL_ID_CDED].hm_icc_sdm_err_submit_req.recv_cmd_code =
        ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(ACCEL_ID_CDED);
    accel_cper_info[ACCEL_ID_CDED].hm_icc_sdm_err_submit_req.payload_buffer = &accel_cper_info[ACCEL_ID_CDED].msg_payload;
    accel_cper_info[ACCEL_ID_CDED].hm_icc_sdm_err_submit_req.buffer_size = sizeof(hm_accel_msg_t);
    accel_cper_info[ACCEL_ID_CDED].hm_icc_sdm_err_submit_req.cb = hm_accel_error_record_submit_listener_cb;
    accel_cper_info[ACCEL_ID_CDED].hm_icc_sdm_err_submit_req.cb_ctx = &accel_cper_info[ACCEL_ID_CDED];
    accel_cper_info[ACCEL_ID_CDED].accel_id = ACCEL_ID_CDED;

    hm_icc_accel_err_register_recv_req[ACCEL_ID_CDED].recv_cmd_code = ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(ACCEL_ID_CDED);
    hm_icc_accel_err_register_recv_req[ACCEL_ID_CDED].payload_buffer =
        (void*)&hm_icc_accel_err_register_payload[ACCEL_ID_CDED];
    hm_icc_accel_err_register_recv_req[ACCEL_ID_CDED].buffer_size = sizeof(hm_accel_error_domain_register_payload_t);
    hm_icc_accel_err_register_recv_req[ACCEL_ID_CDED].cb = hm_cded_sdm_error_domain_register_listener_cb;
    hm_icc_accel_err_register_recv_req[ACCEL_ID_CDED].cb_ctx = (void*)&hm_icc_accel_err_register_payload[ACCEL_ID_CDED];

    fpfw_status_t status = fpfw_icc_base_recv(icc_ctx, &hm_icc_accel_err_register_recv_req[ACCEL_ID_CDED]);
    BUG_ASSERT(status == FPFW_ICC_BASE_STATUS_SUCCESS);
}