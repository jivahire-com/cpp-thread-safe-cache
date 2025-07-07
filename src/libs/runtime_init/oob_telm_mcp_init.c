//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file oob_telm_mcp_init.c
 * Initialize the OOB telemetry MCP components.
 */
/*------------- Includes -----------------*/
#include <DfwkThreadXHost.h>
#include <FpFwUtils.h>
#include <fpfw_dw_i3c.h>           // for fpfw_dw_i3c_config_t, fpfw_dw_i3c_device_t
#include <fpfw_init.h>             // for FPFW_INIT_COMPONENT, FPFW_INIT_D...
#include <fpfw_mctp.h>             // for fpfw_mctp_config, fpfw_mctp_i3c_binding_config
#include <fpfw_mctp_i3c_binding.h> // for fpfw_mctp_i3c_binding_ctx
#include <fpfw_pldm_service.h>     // for pldm_service_config_t, pldm_platform_event_ready_notification
#include <idsw_kng.h>
#include <interrupts.h>
#include <mcp_exp_top_regs.h>
#include <pldm_pdr.h> // for pldm_pdr_timestamp_t
#include <silibs_mcp_top_regs.h>

/*------------- Typedefs -----------------*/

/**
 * This is the initial static address configured by the MCP for the I3C target.
 * After discovery, the target will be assigned a dynamic address by the controller.
 */
#define TARGET_ADDRESS (0xDE)

/* Microsoft MIPI vendor ID is 0x02CB https://mid.mipi.org/  */
#define MSFT_MFR_ID (0x02CB)

// TODO: Update this to the correct PID for Kingsgate once integrated into BMC.
// https://azurecsi.visualstudio.com/Dev/_workitems/edit/2653165
#define MSFT_TARGET_PID 0x12345678

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static uint32_t s_target_response_buf[8];
static uint32_t s_target_rx_data_buf[128];

/*------------- Functions ----------------*/
FPFW_INIT_COMPONENT(i3c_target, FPFW_INIT_DEPENDENCIES("hw_ver", "nvic", "dfwk", "gpio_lib"))
{
    DEBUG_PRINT("I3C target init \n");
    if (idsw_get_die_id() == DIE_1)
    {
        return fpfw_init_skip();
    }

    static fpfw_dw_i3c_device_t s_i3c_target_device;
    static fpfw_dw_i3c_config_t s_i3c_target_config = {
        // 0x1070000U + 0x3d0000U = 0x1440000U
        .dw_i3c_config.register_base_addr = MCP_TOP_MCP_EXP_ADDRESS + MSCP_EXP_TOP_I3C_2_ADDRESS,
        .dw_i3c_config.address = TARGET_ADDRESS,
        .dw_i3c_config.irq_num = HW_INT_I3C_CTRL_2_INT,
        .dw_i3c_config.index = 2,
        .dw_i3c_config.slv_mfg_id = MSFT_MFR_ID,
        .dw_i3c_config.slv_pid = MSFT_TARGET_PID,
        .dw_i3c_config.slv_dcr = 0xCC,
        // 0xC3 = 0b11000011 [i3c device node]
        // 0xCC = 0b11001100 [mctp device node]
        .dw_i3c_config.slv_rx_resp_buf = (uintptr_t)s_target_response_buf,
        .dw_i3c_config.slv_rx_resp_buf_len = sizeof(s_target_response_buf),
        .dw_i3c_config.slv_rx_data_buf = (uintptr_t)s_target_rx_data_buf,
        .dw_i3c_config.slv_rx_data_buf_len = sizeof(s_target_rx_data_buf),
        .device_name = "I3C_2 Target Device"};

    fpfw_status_t status = fpfw_dw_i3c_initialize(&s_i3c_target_device, &s_i3c_target_config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }
    DfwkDeviceInitialize(&s_i3c_target_device.header, &((PDFWK_THREADX_HOST)fpfw_init_get_handle("dfwk"))->Schedule);
    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_i3c_target_device};
}

#define NUM_MCTP_PACKETS    8
#define NUM_MSG_TYPES       1
#define NUM_TX_MCTP_PACKETS 2
#define MSG_TYPE_PLDM       0x01
#define PLDM_VERSION        0xF1F10000

FPFW_INIT_COMPONENT(mctp, FPFW_INIT_DEPENDENCIES("i3c_target"))
{
    static fpfw_mctp mctp_context;
    static fpfw_mctp_i3c_binding_ctx mctp_i3c_binding_ctx = {0};
    static fpfw_dw_i3c_interface_t i3c_target_interface;
    static uint8_t s_msg_types[] = {MSG_TYPE_PLDM};
    static uint32_t s_msg_versions[] = {PLDM_VERSION};

    fpfw_dw_i3c_device_t* i3c_target_dev = (fpfw_dw_i3c_device_t*)fpfw_init_get_handle("i3c_target");
    fpfw_dw_i3c_interface_initialize(i3c_target_dev, &i3c_target_interface);

    static fpfw_mctp_i3c_packet_info s_mctp_i3c_packets[NUM_MCTP_PACKETS] = {0};
    static fpfw_mctp_i3c_binding_config mctp_i3c_binding_config = {.name = "mctp_i3c_binding",
                                                                   .i3c_interface = &i3c_target_interface.i3c_interface,
                                                                   .mctp_ctx = &mctp_context,
                                                                   .packet_array = s_mctp_i3c_packets,
                                                                   .num_packets = NUM_MCTP_PACKETS,
                                                                   .num_pending_rx_requests = 0};
    fpfw_status_t status = fpfw_mctp_i3c_binding_init(&mctp_i3c_binding_ctx, &mctp_i3c_binding_config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    static fpfw_mctp_config mctp_config = {.mctp_instance_name = "MCTP0",
                                           .message_support_len = NUM_MSG_TYPES,
                                           .binding = {.mctp_binding_ctx = &mctp_i3c_binding_ctx,
                                                       .send_pkt = fpfw_mctp_i3c_binding_send_pkt,
                                                       .get_pkt = fpfw_mctp_i3c_binding_get_pkt,
                                                       .release_pkt = fpfw_mctp_i3c_binding_release_pkt},
                                           .max_num_tx_packets = NUM_TX_MCTP_PACKETS};
    mctp_config.message_type_support = s_msg_types;
    mctp_config.message_version_support = s_msg_versions;

    status = fpfw_mctp_init(&mctp_context, &mctp_config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &mctp_context};
}

#define PLDM_NUM_INCOMING_MSGS 7
#define PLDM_INCOMING_MSG_SIZE 0x100
#define PLDM_NUM_OUTGOING_MSGS 7
#define PLDM_OUTGOING_MSG_SIZE 0x400

static void platform_event_ready_callback(uint16_t event_id, void* context)
{
    uint16_t* last_event_id = (uint16_t*)context;
    *last_event_id = event_id;
    DEBUG_PRINT("PLDM platform event ready callback called with event ID: %u\n", event_id);
}

#define SYS_THREAD_STACK_SIZE_PLDM_SERVICE ((TX_MINIMUM_STACK) + ((2) * (FPFW_KB)))
#define SYS_THREAD_TIME_SLICE_PLDM_SERVICE (TX_NO_TIME_SLICE)
// TODO: Modify priority later https://azurecsi.visualstudio.com/Dev/_workitems/edit/2743962
#define MCP_THREAD_PRIORITY_PLDM_SERVICE (3)

FPFW_INIT_COMPONENT(pldm, FPFW_INIT_DEPENDENCIES("mctp", "pdr_repo"))
{
    static uint16_t last_event_id = 0;
    static pldm_platform_event_ready_notification pe_ready_notification = {
        .CallBack = platform_event_ready_callback,
        .context = &last_event_id,
    };

    static uint8_t pldm_service_stack[SYS_THREAD_STACK_SIZE_PLDM_SERVICE];

    static uint8_t pldm_rx_msgs[PLDM_MESSAGE_BUFFER_SIZE(PLDM_INCOMING_MSG_SIZE, PLDM_NUM_INCOMING_MSGS)]
        __attribute__((aligned(8)));
    static uint8_t pldm_tx_msgs[PLDM_MESSAGE_BUFFER_SIZE(PLDM_OUTGOING_MSG_SIZE, PLDM_NUM_OUTGOING_MSGS)]
        __attribute__((aligned(8)));

    static pldm_service_config_t pldmConfig = {.thread_config = {.p_stack = (void*)pldm_service_stack,
                                                                 .stack_size = SYS_THREAD_STACK_SIZE_PLDM_SERVICE,
                                                                 .priority = MCP_THREAD_PRIORITY_PLDM_SERVICE,
                                                                 .should_yield = true,
                                                                 .time_slice_option = SYS_THREAD_TIME_SLICE_PLDM_SERVICE},

                                               .incoming_messages = (void*)pldm_rx_msgs,
                                               .incoming_msg_payload_size = PLDM_INCOMING_MSG_SIZE,
                                               .num_incoming_messages = PLDM_NUM_INCOMING_MSGS,

                                               .outgoing_messages = (void*)pldm_tx_msgs,
                                               .outgoing_msg_payload_size = PLDM_OUTGOING_MSG_SIZE,
                                               .num_outgoing_messages = PLDM_NUM_OUTGOING_MSGS,
                                               .max_platform_event_size = PLDM_OUTGOING_MSG_SIZE};

    pldmConfig.mctp_ctx = (fpfw_mctp*)fpfw_init_get_handle("mctp");
    pldmConfig.pdr_repo = (void*)fpfw_init_get_handle("pdr_repo");

    pldm_pdr_timestamp_t time = pldm_pdr_get_timestamp();
    pldmConfig.pdr_config.repo_timestamp = time.as_raw;
    pldmConfig.pdr_config.oam_timestamp = time.as_raw;

    pldmConfig.pdr_config.UUID = (fpfw_pldm_uuid_t){0};

    fpfw_status_t status = fpfw_pldm_service_init(&pldmConfig);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    fpfw_pldm_service_register_platform_event_ready_notification(&pe_ready_notification);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
