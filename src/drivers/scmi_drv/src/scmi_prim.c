//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file scmi_prim.c
 *  This modules supports scmi functions as primitives
 */

/*------------- Includes -----------------*/
#include "scmi_init.h"
#include "scmi_prim_i.h"

#include <DfwkClient.h>
#include <FpFwAssert.h>
#include <FpFwCli.h> // for SCMI_LOG_INFO, _FPFW_CLI_STATUS, FPF...
#include <ap_core.h>
#include <arm_intrinsic.h>
#include <atu_api.h>
#include <bug_check.h> // for BUG_ASSERT
#include <cmsis_m7.h>
#include <debug.h>
#include <fpfw_icc_transport_interface.h>
#include <fpfw_init.h>
#include <fpfw_status.h>
#include <icc_mhu.h>
#include <inttypes.h>
#include <kng_icc_shared.h>
#include <kng_scmi_shared.h>
#include <kng_scp_tfa_shared.h>
#include <mhu_icc_transport.h>
#include <scmi_prim.h>
#include <startup_shutdown.h>
#include <startup_shutdown_init.h>
#include <stdint.h>
#include <string.h>
#include <system_info.h>

#define BIT_32(nr) (((uint32_t)(1U)) << (nr))
#define SMT_CHANNEL_FREE \
    BIT_32(0) //! Set 0th bit of status to indicate channel is free ie when SMT does not contain pending message

#define SET_SCMI_SMT_CHANNEL_FREE(status) ((status) |= BIT_32(0))
#define SCMI_SMT_IS_CHANNEL_FREE(status)  ((status) & BIT_32(0))

// Local data structures
typedef struct
{
    smt_header_t smt_header;
    scmi_message_header_t header;
    uint32_t payload[12];
} scmi_local_packet_t;

typedef struct
{
    icc_mhu_header_t header;
    uint8_t data[64];
} scmi_transport_message_t;

static DFWK_INTERFACE_HEADER* p_apcore_interface = NULL;
static ap_core_asynchronous_request_t apcore_request;
static uint8_t scmi_debug = 0x0;

static mhu_icc_transport_intrf_t* s_icc_mscp2tfa = NULL;
static volatile scmi_transport_message_t* scmi_recv_message;
static volatile scmi_transport_message_t* scmi_send_message;
static FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST recv_request;

static startup_shutdown_request_t scmi_sys_pwr_set_req;

void scmi_cold_boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    FPFW_UNUSED(request);
    printf("Cold Boot Completed\n");
}

void scmi_shutdown_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    FPFW_UNUSED(request);
    printf("Shutdown Completed\n");
}

void scmi_warm_boot_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    printf("Request (%x)\n is completed\n", (uintptr_t)request);
    printf("AP Warm Boot Completed\n");
}

void scmi_set_debug_mode(uint8_t mode)
{
    scmi_debug = mode;
}

void ap_core_power_response(int32_t status)
{
    scmi_pd_power_state_notify_p2a_t resp;
    resp.status = status;
    scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
    SCMI_LOG_INFO("Sent SCMI_PWR_STATE_SET_MSG response: %d\n", (int)resp.status);
}

void ap_core_power_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(p_completion_context);
    BUG_ASSERT_PARAM(request != NULL, request, 0);

    if (request->RequestType == APCORE_CORE_POWER_ON_ASYNC)
    {
        //! Only send scmi response for power on request
        ap_core_power_response(SCMI_STATUS_SUCCESS);
        SCMI_LOG_INFO("Apcore Power On Request Completed!\n");
    }
    else
    {
        //! response for power off request was already send before initiating the power off sequence
        SCMI_LOG_INFO("Apcore Power Off Request Completed!\n");
    }
}

void ap_core_power(uint32_t power_domain, uint32_t power_state)
{
    // ensure the apcore interface was set
    BUG_ASSERT_PARAM(p_apcore_interface != NULL, p_apcore_interface, 0);

    DfwkAsyncRequestInitialize(&apcore_request.header, sizeof(apcore_request));
    if ((power_state & SCMI_PD_CORE_STATE_MASK) == (SCMI_PD_CORE_STATE_OFF))
    {
        //! Received power off request from AP core over SCMI ap core protocol
        //! SCP must send scmi resp 1st, as AP core is waiting for response (before it proceeds set the power state & call wfi)
        ap_core_power_response(SCMI_STATUS_SUCCESS);
        SCMI_LOG_INFO("Apcore Power Off SCMI Response Sent!\n");
        //! SCP then programs the PPU registers with power off state & operating mode policy (via the `APCORE_CORE_POWER_OFF_ASYNC` req below)
        //! SCP then waits for the PPU status reg to be updated with a timeout before completing the async request
        //! Concurrently, AP will then Set the power state (which updates the PPU status register) & call wfi, concluding the power off sequence
        ap_core_core_power_off(p_apcore_interface, &apcore_request, power_domain, ap_core_power_completion, NULL);
    }
    else if ((power_state & SCMI_PD_CORE_STATE_MASK) == (SCMI_PD_CORE_STATE_ON))
    {
        ap_core_core_power_on(p_apcore_interface, &apcore_request, power_domain, ap_core_power_completion, NULL);
    }
    else
    {
        SCMI_LOG_INFO("Invalid Power State: %" PRIx32 "\n", power_state);
        ap_core_power_response(SCMI_STATUS_INVALID_PARAMETERS);
    }
}

int scmi_power_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size)
{
    int status = SCMI_PROTOCOL_CMD_SUCCESS;

    SCMI_LOG_INFO("Processing SCMI Power Protocol Cmd: %x\n", cmd_code);
    if (size != 0 && ((scmi_debug & 0x04) != 0))
    {
        for (size_t data_count = 0; data_count < size; data_count++)
        {
            SCMI_LOG_INFO(" scmi_message data[%zu]: %x\n", data_count, payload[data_count]);
        }
    }

    // Call the Power domian request from here if the command is valid
    switch (cmd_code)
    {

    case SCMI_PROTOCOL_VERSION: {
        scmi_protocol_version_resp_t resp;
        resp.status = 0;
        resp.version = 0x2000;
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTOCOL_VERSION, (uint8_t*)&resp, sizeof(resp));
        SCMI_LOG_INFO("SCMI_PWR_PROTOCOL_VERSION: %x\n", cmd_code);
        break;
    }
    case SCMI_PROTO_MSG_ATTR_MSG: {
        if (size < sizeof(scmi_protocol_message_attributes_a2p_t))
        {
            SCMI_LOG_ERR("SCMI_PROTO_MSG_ATTR_MSG: invalid payload size %zu\n", size);
            scmi_protocol_message_attributes_p2a_t resp = {0};
            resp.status = SCMI_STATUS_INVALID_PARAMETERS;
            scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTO_MSG_ATTR_MSG, (uint8_t*)&resp, sizeof(resp));
            break;
        }
        scmi_protocol_message_attributes_a2p_t* p_message_attr = (scmi_protocol_message_attributes_a2p_t*)payload;
        SCMI_LOG_INFO("SCMI_PROTO_ATTR_MSG: %x message_id %" PRIx32 "\n", cmd_code, p_message_attr->message_id);
        scmi_protocol_message_attributes_p2a_t resp;
        resp.status = SCMI_STATUS_SUCCESS;
        resp.attributes = 0;
        switch (p_message_attr->message_id)
        {
        // supported messages here
        case SCMI_PWR_STATE_SET_MSG:
            SCMI_LOG_INFO("SCMI_PWR_STATE_SET_MSG\n");
            break;
        // everything else is unsupported
        default:
            resp.status = SCMI_STATUS_NOT_FOUND;
            break;
        }
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: returning %" PRId32 " (attr: %" PRIx32 ")\n", resp.status, resp.attributes);
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTO_MSG_ATTR_MSG, (uint8_t*)&resp, sizeof(resp));
        break;
    }
    case SCMI_PWR_STATE_SET_MSG: {
        SCMI_LOG_INFO("SCMI_PWR_STATE_SET_MSG: %x\n", cmd_code);
        if (size < sizeof(scmi_pd_power_state_set_a2p_t))
        {
            SCMI_LOG_ERR("SCMI_PWR_STATE_SET_MSG: invalid payload size %zu\n", size);
            ap_core_power_response(SCMI_STATUS_INVALID_PARAMETERS);
            break;
        }
        //! Do we not care about flags?
        scmi_pd_power_state_set_a2p_t power_state;
        // because alignment :(
        memcpy(&power_state, payload, sizeof(power_state));
        ap_core_power(power_state.domain_id, power_state.power_state);
        break;
    }
    case SCMI_PWR_STATE_GET_MSG:
        SCMI_LOG_INFO("SCMI_PWR_STATE_GET_MSG: %x\n", cmd_code);
        scmi_pwr_domain_get_state_resp_t resp;
        resp.status = 0;
        resp.power_state = 1;
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PWR_STATE_GET_MSG, (uint8_t*)&resp, sizeof(resp));
        break;

    default:
        SCMI_LOG_INFO("Unknown Power Domain Protocol Command: %x \n", cmd_code);
        status = SCMI_PROTOCOL_CMD_UKNOWN;
        break;
    }

    return status;
}

int scmi_sys_pwr_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size)
{
    int status = SCMI_PROTOCOL_CMD_SUCCESS;
    SCMI_LOG_INFO("Processing SCMI Power Protocol Cmd: %x\n", cmd_code);
    if (size != 0 && ((scmi_debug & 0x04) != 0))
    {
        for (size_t data_count = 0; data_count < size; data_count++)
        {
            SCMI_LOG_INFO(" scmi_message data[%zu]: %x\n", data_count, payload[data_count]);
        }
    }

    // Call the Sys Power request from here if the command is valid
    switch (cmd_code)
    {

    case SCMI_PROTOCOL_VERSION: {
        scmi_protocol_version_resp_t resp;
        resp.status = 0;
        resp.version = 0x2000;
        scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PROTOCOL_VERSION, (uint8_t*)&resp, sizeof(resp));
        SCMI_LOG_INFO("SCMI_SYS_PWR_PROTOCOL_VERSION: %x\n", cmd_code);
        break;
    }
    case SCMI_PROTO_MSG_ATTR_MSG: {
        if (size < sizeof(scmi_protocol_message_attributes_a2p_t))
        {
            SCMI_LOG_ERR("SCMI_PROTO_MSG_ATTR_MSG: invalid payload size %zu\n", size);
            scmi_protocol_message_attributes_p2a_t resp = {0};
            resp.status = SCMI_STATUS_INVALID_PARAMETERS;
            scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PROTO_MSG_ATTR_MSG, (uint8_t*)&resp, sizeof(resp));
            break;
        }
        scmi_protocol_message_attributes_a2p_t* p_message_attr = (scmi_protocol_message_attributes_a2p_t*)payload;
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: %x message_id %" PRIx32 "\n", cmd_code, p_message_attr->message_id);
        scmi_protocol_message_attributes_p2a_t resp;
        resp.status = SCMI_STATUS_SUCCESS;
        resp.attributes = 0;
        switch (p_message_attr->message_id)
        {
        // supported messages here
        case SCMI_SYS_PWR_STATE_SET_MSG:
            break;
        // everything else is unsupported
        default:
            resp.status = SCMI_STATUS_NOT_FOUND;
            break;
        }
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: returning %" PRId32 " (attr: %" PRIx32 ")\n", resp.status, resp.attributes);
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTO_MSG_ATTR_MSG, (uint8_t*)&resp, sizeof(resp));
        break;
    }
    case SCMI_SYS_PWR_STATE_SET_MSG: {
        SCMI_LOG_INFO("SCMI_SYS_PWR_STATE_SET_MSG: %x\n", cmd_code);

        if (size < sizeof(scmi_sys_pwr_set_state_a2p_t))
        {
            SCMI_LOG_ERR("SCMI_SYS_PWR_STATE_SET_MSG: invalid payload size %zu\n", size);
            scmi_sys_pwr_set_state_p2a_t resp = {0};
            resp.status = SCMI_STATUS_INVALID_PARAMETERS;
            scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_SYS_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
            break;
        }

        scmi_sys_pwr_set_state_a2p_t* p_sys_pwr_set_state = (scmi_sys_pwr_set_state_a2p_t*)payload;
        bool ignore_pwr_up = (bool)(p_sys_pwr_set_state->flags & 0x1U);
        scmi_sys_pwr_set_state_p2a_t resp = {0};
        resp.status = SCMI_STATUS_SUCCESS;

        if (p_sys_pwr_set_state->flags & (uint32_t)(~0x1U))
        {
            resp.status = SCMI_STATUS_INVALID_PARAMETERS;
            scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
        }
        else
        {
            // send synchronous and asynchronous startup stage start requests
            DfwkAsyncRequestInitialize((void*)&scmi_sys_pwr_set_req.header, sizeof(scmi_sys_pwr_set_req));

            switch (p_sys_pwr_set_state->system_state)
            {
            case SCMI_SYS_PWR_SYS_STATE_SHUTDOWN:
                scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
                sos_shutdown((void*)fpfw_init_get_handle("sos_int"), &scmi_sys_pwr_set_req, SHUTDOWN, scmi_shutdown_completion, NULL);
                break;
            case SCMI_SYS_PWR_SYS_STATE_COLD_RESET:
                scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
                sos_shutdown((void*)fpfw_init_get_handle("sos_int"), &scmi_sys_pwr_set_req, COLD_RESET, scmi_cold_boot_completion, NULL);
                break;
            case SCMI_SYS_PWR_SYS_STATE_WARM_RESET:
                scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
                sos_shutdown((void*)fpfw_init_get_handle("sos_int"), &scmi_sys_pwr_set_req, AP_WARM_RESET, scmi_warm_boot_completion, NULL);
                break;
            case SCMI_SYS_PWR_SYS_STATE_POWER_UP:
                scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
                if (!ignore_pwr_up)
                {
                    SCMI_LOG_INFO("Force Power Up\n");
                }
                break;
            case SCMI_SYS_PWR_SYS_STATE_SUSPEND:
                scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
                break;
            default:
                SCMI_LOG_INFO("SCMI_SYS_PWR_STATE_SET_MSG: invalid system_state : %" PRId32 "\n",
                              p_sys_pwr_set_state->system_state);

                resp.status = SCMI_STATUS_INVALID_PARAMETERS;
                SCMI_LOG_INFO("SCMI_SYS_PWR_STATE_SET_MSG: returning %" PRId32 "\n", resp.status);
                scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_PWR_STATE_SET_MSG, (uint8_t*)&resp, sizeof(resp));
                break;
            }
        }
        break;
    }
    case SCMI_SYS_PWR_STATE_GET_MSG: {
        SCMI_LOG_INFO("SCMI_SYS_PWR_STATE_GET_MSG: %x\n", cmd_code);

        // As an example, return system power up 0x3 as the system state
        scmi_sys_pwr_get_state_resp_t resp;
        resp.status = 0;
        resp.system_state = 3;
        scmi_send_resp(SCMI_SYS_PWR_PROTO_ID, SCMI_SYS_PWR_STATE_GET_MSG, (uint8_t*)&resp, sizeof(resp));
        break;
    }
    default:
        SCMI_LOG_INFO("Unknown Sys Power Command: %x \n", cmd_code);
        status = SCMI_PROTOCOL_CMD_UKNOWN;
        break;
    }

    return status;
}

void ap_core_reset_addr_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context)
{
    FPFW_UNUSED(request);
    FPFW_UNUSED(p_completion_context);
    scmi_apcore_reset_address_set_p2a_t resp;
    resp.status = SCMI_STATUS_SUCCESS;
    scmi_send_resp(SCMI_AP_CORE_PROTO_ID, SCMI_AP_CORE_RESET_ADDR_SET_MSG, (uint8_t*)&resp, sizeof(resp));
    SCMI_LOG_INFO("Sent SCMI_AP_CORE_RESET_ADDR_SET_MSG response: %d\n", (int)resp.status);
}

void ap_core_reset_addr_set(uint64_t reset_address)
{
    // ensure the apcore interface was set
    BUG_ASSERT_PARAM(p_apcore_interface != NULL, p_apcore_interface, 0);

    DfwkAsyncRequestInitialize(&apcore_request.header, sizeof(apcore_request));
    ap_core_set_rvbaraddr(p_apcore_interface, &apcore_request, reset_address, ap_core_reset_addr_completion, NULL);
}

int scmi_ap_core_protocol_cmds(uint8_t cmd_code, uint8_t* payload, size_t size)
{
    int status = SCMI_PROTOCOL_CMD_SUCCESS;
    // Parse the command code
    SCMI_LOG_INFO("Processing AP Core Protocol Cmd: %d\n", cmd_code);
    if (size != 0 && ((scmi_debug & 0x04) != 0))
    {
        for (size_t data_count = 0; data_count < size; data_count++)
        {
            SCMI_LOG_INFO(" scmi_message data[%zu]: %x\n", data_count, payload[data_count]);
        }
    }

    // Call the  AP Core request from here if the command is valid
    switch (cmd_code)
    {
    case SCMI_PROTOCOL_VERSION: {
        scmi_protocol_version_resp_t resp;
        resp.status = 0;
        resp.version = 0x2000;
        scmi_send_resp(SCMI_AP_CORE_PROTO_ID, SCMI_PROTOCOL_VERSION, (uint8_t*)&resp, sizeof(resp));
        SCMI_LOG_INFO("SCMI_AP_CORE_PROTOCOL_VERSION: %x\n", cmd_code);
        break;
    }
    case SCMI_PROTO_MSG_ATTR_MSG: {
        if (size < sizeof(scmi_protocol_message_attributes_a2p_t))
        {
            SCMI_LOG_ERR("SCMI_PROTO_MSG_ATTR_MSG: invalid payload size %zu\n", size);
            scmi_protocol_message_attributes_p2a_t resp = {0};
            resp.status = SCMI_STATUS_INVALID_PARAMETERS;
            scmi_send_resp(SCMI_AP_CORE_PROTO_ID, SCMI_PROTO_MSG_ATTR_MSG, (uint8_t*)&resp, sizeof(resp));
            break;
        }
        scmi_protocol_message_attributes_a2p_t* p_message_attr = (scmi_protocol_message_attributes_a2p_t*)payload;
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: %x message_id %" PRIx32 "\n", cmd_code, p_message_attr->message_id);
        scmi_protocol_message_attributes_p2a_t resp;
        resp.status = SCMI_STATUS_SUCCESS;
        resp.attributes = 0;
        switch (p_message_attr->message_id)
        {
        // supported messages here
        case SCMI_AP_CORE_RESET_ADDR_SET_MSG:
            break;
        // everything else is unsupported
        default:
            resp.status = SCMI_STATUS_NOT_FOUND;
            break;
        }
        SCMI_LOG_INFO("SCMI_PROTO_MSG_ATTR_MSG: returning %d (attr: %" PRIx32 ")\n", (int)resp.status, resp.attributes);
        scmi_send_resp(SCMI_PWR_DMN_PROTO_ID, SCMI_PROTO_MSG_ATTR_MSG, (uint8_t*)&resp, sizeof(resp));
        break;
    }
    case SCMI_AP_CORE_RESET_ADDR_SET_MSG: {
        if (size < sizeof(scmi_apcore_reset_address_set_a2p_t))
        {
            SCMI_LOG_ERR("SCMI_AP_CORE_RESET_ADDR_SET_MSG: invalid payload size %zu\n", size);
            scmi_apcore_reset_address_set_p2a_t resp = {0};
            resp.status = SCMI_STATUS_INVALID_PARAMETERS;
            scmi_send_resp(SCMI_AP_CORE_PROTO_ID, SCMI_AP_CORE_RESET_ADDR_SET_MSG, (uint8_t*)&resp, sizeof(resp));
            break;
        }
        scmi_apcore_reset_address_set_a2p_t reset_address;
        memcpy(&reset_address, payload, sizeof(reset_address));
        SCMI_LOG_INFO("SCMI_AP_CORE_RESET_ADDR_SET_MSG: %x high: %08" PRIx32 " low: %08" PRIx32
                      " attr: %08" PRIx32 "\n",
                      cmd_code,
                      reset_address.reset_address_high,
                      reset_address.reset_address_low,
                      reset_address.attributes);
        ap_core_reset_addr_set(((uint64_t)reset_address.reset_address_high << 32) | reset_address.reset_address_low);
    }
    break;

    case SCMI_AP_CORE_RESET_ADDR_GET_MSG:
        SCMI_LOG_INFO("SCMI_AP_CORE_RESET_ADDR_GET_MSG: %x\n", cmd_code);
        break;

    default:
        SCMI_LOG_INFO("Unknown AP Core Command: %x \n", cmd_code);
        status = SCMI_PROTOCOL_CMD_UKNOWN;
        break;
    }
    return status;
}

int scmi_check_message(scmi_icc_packet_t* packet)
{
    int status = SCMI_PROTOCOL_CMD_SUCCESS;
    if ((scmi_debug & 1) != 0)
    {
        // Spew SCMI message
        SCMI_LOG_INFO("SMT Status      : %d\n", (int)packet->smt_header.status);
        SCMI_LOG_INFO("SMT Flags       : %d\n", (int)packet->smt_header.flags);
        SCMI_LOG_INFO("SMT Payload Size: %d\n", (int)packet->smt_header.payload_size);
        SCMI_LOG_INFO("SCMI Protocol ID: 0x %x\n", (int)packet->header.protocol_id);
        SCMI_LOG_INFO("SCMI Message  ID: 0x %x\n", (int)packet->header.cmd_code);
    }

    const size_t header_size = sizeof(packet->header);
    const size_t max_payload_size = sizeof(((scmi_local_packet_t*)0)->payload);
    if ((size_t)packet->smt_header.payload_size < header_size)
    {
        SCMI_LOG_INFO("SCMI payload_size (%zu) too small for header (%zu)\n", (size_t)packet->smt_header.payload_size, header_size);
        return SCMI_PROTOCOL_CMD_UKNOWN;
    }
    const size_t payload_data_size = (size_t)packet->smt_header.payload_size - header_size;
    if (payload_data_size > max_payload_size)
    {
        SCMI_LOG_INFO("SCMI payload_data_size (%zu) exceeds max payload buffer (%zu)\n", payload_data_size, max_payload_size);
        return SCMI_PROTOCOL_CMD_UKNOWN;
    }

    // Check for the appropriate action
    switch (packet->header.protocol_id)
    {

    case SCMI_PWR_DMN_PROTO_ID:
        status = scmi_power_protocol_cmds(packet->header.cmd_code, (uint8_t*)packet->payload, payload_data_size);
        break;

    case SCMI_SYS_PWR_PROTO_ID:
        status = scmi_sys_pwr_protocol_cmds(packet->header.cmd_code, (uint8_t*)packet->payload, payload_data_size);
        break;

    case SCMI_AP_CORE_PROTO_ID:
        status = scmi_ap_core_protocol_cmds(packet->header.cmd_code, (uint8_t*)packet->payload, payload_data_size);
        break;

    default:
        SCMI_LOG_INFO("SCMI Protocol  ID not supported \n");
        status = SCMI_PROTOCOL_NOT_SUPPORTED;
        break;
    }

    return status;
}

int scmi_send_resp(uint8_t protocol_id, uint8_t cmd_id, uint8_t* payload, size_t size)
{
    scmi_send_message->header.msg_header.command = ICC_SCMI_CLIENT_MOD_RESP;
    scmi_send_message->header.msg_header.payload_size = sizeof(scmi_local_packet_t);
    // Send the SCMI Response
    scmi_local_packet_t* local_packet = (scmi_local_packet_t*)scmi_send_message->data;
    local_packet->smt_header.flags = 1;
    local_packet->smt_header.status = 1;
    local_packet->smt_header.payload_size = size + sizeof(local_packet->header);
    local_packet->header.protocol_id = protocol_id;
    local_packet->header.msg_type = cmd_id;
    local_packet->header.token = 0;

    memcpy(local_packet->payload, payload, size);
    __DSB();

    fpfw_status_t status = fpfw_icc_transport_try_send_sync_req(&(s_icc_mscp2tfa->base_interface),
                                                                (void*)scmi_send_message,
                                                                sizeof(scmi_transport_message_t));

    return status;
}

void scmi_set_apcore_interface(DFWK_INTERFACE_HEADER* p_interface)
{
    // save the interface off for later use
    p_apcore_interface = p_interface;
    DfwkClientInterfaceOpen(p_interface);
}

static void scmi_async_recv_completion(PDFWK_ASYNC_REQUEST_HEADER Request, void* Context)
{
    FPFW_UNUSED(Context);
    BUG_ASSERT_PARAM(NULL != Request, Request, 0);

    PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST request = (PFPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST)Request;
    volatile scmi_transport_message_t* recv_msg = (volatile scmi_transport_message_t*)request->Input.PayloadBuffer;
    volatile scmi_local_packet_t* shared_memory_packet = (volatile scmi_local_packet_t*)recv_msg->data;

    SCMI_LOG_INFO("SCMI Async Recv Completion Raised!\n");
    //! Ensure that the channel is busy here, tfa marks the scmi channel busy
    //! before it sends icc message to scp
    BUG_ASSERT_PARAM(!SCMI_SMT_IS_CHANNEL_FREE(shared_memory_packet->smt_header.status),
                     shared_memory_packet->smt_header.status,
                     0);

    //! Check if the request was successful & received some bytes
    if ((request->Output.Status == FPFW_ICC_TRANSPORT_STATUS_SUCCESS) && (request->Output.ReceivedBytes > 0))
    {
        //! Check if the message is a SCMI message
        if (recv_msg->header.msg_header.command == ICC_SCMI_HOST_MODULE_SEND)
        {
            //! Copy shared-memory packet to a private stack buffer
            scmi_local_packet_t local_packet;
            memcpy(&local_packet, (const void*)shared_memory_packet, sizeof(local_packet));

            //! Update the scmi status bit to free channel, pseudo ack to the tfa
            //! tfa will spin until the channel is free before it proceeds to loop until mesg received
            SET_SCMI_SMT_CHANNEL_FREE(shared_memory_packet->smt_header.status);
            __DSB();

            //! spew the messages if debug is enabled (reads from local_packet, not shared memory)
            if ((scmi_debug & 2) != 0)
            {
                SCMI_LOG_INFO("SCMI ICC Message: %x\n", (int)scmi_recv_message->header.msg_header.command);
                SCMI_LOG_INFO("SCMI ICC Size: %x\n", (int)scmi_recv_message->header.msg_header.payload_size);

                if (scmi_recv_message->header.msg_header.payload_size != 0 &&
                    scmi_recv_message->header.msg_header.payload_size <= sizeof(scmi_recv_message->data) &&
                    (scmi_debug & 8) != 0)
                {
                    for (size_t data_count = 0; data_count < scmi_recv_message->header.msg_header.payload_size; data_count++)
                    {
                        SCMI_LOG_INFO("  scmi_message data[%zu]: %x\n", data_count, ((const uint8_t*)&local_packet)[data_count]);
                    }
                }
            }
            BUG_ASSERT_PARAM(scmi_recv_message->header.msg_header.payload_size <=
                                 sizeof(((scmi_local_packet_t*)0)->payload),
                             scmi_recv_message->header.msg_header.payload_size,
                             sizeof(((scmi_local_packet_t*)0)->payload));
            //! Parse the message as per SCMI protocol
            scmi_check_message((scmi_icc_packet_t*)&local_packet);
        }
        else
        {
            //! Unexpected! Only SCMI message command expected on scp tfa icc interface!
            BUG_ASSERT_PARAM(0, recv_msg->header.msg_header.command, 0);
        }
    }
    else
    {
        //! Unexpected! Status failed or no bytes received
        BUG_ASSERT_PARAM(0, request->Output.Status, request->Output.ReceivedBytes);
    }

    //! respawn a new async recv request, always keep an async recv request alive
    fpfw_status_t status = fpfw_icc_transport_recv_async_req(&(s_icc_mscp2tfa->base_interface),
                                                             &recv_request.Header,
                                                             (void*)scmi_recv_message,
                                                             sizeof(scmi_transport_message_t),
                                                             scmi_async_recv_completion,
                                                             NULL);
    BUG_ASSERT_PARAM(status == FPFW_ICC_TRANSPORT_STATUS_SUCCESS, status, 0);
}

void scmi_drv_init(DFWK_INTERFACE_HEADER* p_scp_tfa_interface)
{
    //! Initialize the scp_tfa interface & scmi send/recv buffer
    BUG_ASSERT_PARAM(p_scp_tfa_interface != NULL, p_scp_tfa_interface, 0);
    s_icc_mscp2tfa = (mhu_icc_transport_intrf_t*)p_scp_tfa_interface;
    scmi_recv_message = (volatile scmi_transport_message_t*)s_icc_mscp2tfa->device->recv_channel.ch_shared_mem_addr;
    scmi_send_message = (volatile scmi_transport_message_t*)s_icc_mscp2tfa->device->send_channel.ch_shared_mem_addr;

    //! Initialize a request to be sent to the mbox icc transport interface
    DfwkAsyncRequestInitialize(&recv_request.Header, sizeof(FPFW_ICC_TRANSPORT_ASYNC_RECV_REQUEST));

    //! Set the recv channel status bit to reflect channel is free (set to 1), required to recv the very 1st message
    volatile scmi_local_packet_t* local_packet = (volatile scmi_local_packet_t*)scmi_recv_message->data;
    //! skip SCMI channel free setting during warm start as the tfa may have already sent a message
    if (!system_info_is_warm_start())
    {
        SET_SCMI_SMT_CHANNEL_FREE(local_packet->smt_header.status);
    }
    __DSB();

    //! Spawn the 1st async recv request to receive messages over scp_tfa interface
    fpfw_status_t status = fpfw_icc_transport_recv_async_req(&(s_icc_mscp2tfa->base_interface),
                                                             &recv_request.Header,
                                                             (void*)scmi_recv_message,
                                                             sizeof(scmi_transport_message_t),
                                                             scmi_async_recv_completion,
                                                             NULL);
    BUG_ASSERT_PARAM(status == FPFW_ICC_TRANSPORT_STATUS_SUCCESS, status, 0);
}