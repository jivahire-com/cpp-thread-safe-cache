//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file oob_uart_mcp_init.c
 * Initialize the OOB telemetry MCP components.
 */

/*------------- Includes -----------------*/

#include <DbgPrint.h> // for FPFW_DBGPRINT_ALWAYS
#include <DfwkThreadXHost.h>
#include <FpFwUtils.h>
#include <fpfw_cfg_mgr.h>           // for knobs
#include <fpfw_init.h>              // for FPFW_INIT_COMPONENT, FPFW_INIT_D...
#include <fpfw_mctp.h>              // for fpfw_mctp_config, fpfw_mctp_i3c_binding_config
#include <fpfw_mctp_uart_binding.h> // for fpfw_mctp_uart_binding_ctx, fpfw_mctp_uart_binding_config
#include <fpfw_pldm_service.h>      // for pldm_service_config_t, pldm_platform_event_ready_notification
#include <idsw_kng.h>
#include <interrupts.h>
#include <mcp_exp_top_regs.h>
#include <pldm_pdr.h> // for pldm_pdr_timestamp_t
#include <silibs_mcp_top_regs.h>
#include <textio_pl011.h> // for textio_pl011_device_t, textio_pl011_device_initialize

/*------------- Typedefs -----------------*/

#define SVP_UART_APB_FREQUENCY  125000000U
#define FPGA_UART_APB_FREQUENCY 10000000U
#define SOC_UART_APB_FREQUENCY  250000000U

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static uint32_t get_uart_apb_frequency(idsw_plat_id_t sdv_id)
{
    switch (sdv_id)
    {
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        return FPGA_UART_APB_FREQUENCY;
    default:
        return SOC_UART_APB_FREQUENCY;
    }
}

static bool platform_supports_oob(idsw_plat_id_t sdv_id)
{
    switch (sdv_id)
    {
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
    case PLATFORM_RVP_EVT_SILICON:
        return true;
    default:
        return false;
    }
}

/*------------- Functions ----------------*/

#define NUM_MCTP_PACKETS    8
#define NUM_MSG_TYPES       1
#define NUM_TX_MCTP_PACKETS 2
#define NUM_RX_MCTP_PACKETS 2
#define MSG_TYPE_PLDM       0x01
#define PLDM_VERSION        0xF1F10000

FPFW_INIT_COMPONENT(mctp_uart, FPFW_INIT_DEPENDENCIES("dfwk", "nvic", "cfg_mgr"))
{
    idsw_plat_id_t sdv_id = idsw_get_platform_sdv();
    if (idsw_get_die_id() == DIE_1 || !platform_supports_oob(sdv_id) || !config_get_uart_mcp_reassign())
    {
        // Skip initialization on Die 1
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    fpfw_init_component_id_t dfwk_id = "dfwk";
    static textio_pl011_config_t pl011_config = {
        .base_address = MCP_TOP_MCP_UART_ADDRESS,
        .vuart_base_address = 0,
        .interrupt = HW_INT_MCP_UART_INT,
        .vuart_interrupt = 0,
        .baud_rate = 115200,
        .wlen = UART_PL011_WLEN_8,
        .stop_bits = UART_PL011_STOP_BITS_1,
        .parity = UART_PL011_PARITY_NONE,
        .config_type = TEXTIO_PL011_CONFIG_TYPE_INTERRUPT,
        .is_vuart_enabled = false,
        .disable_auto_cr = true, //! prevent textio driver from inserting 0x0D post 0x0A
    };

    pl011_config.clk_freq = get_uart_apb_frequency(sdv_id); // Set clock frequency from APB clock

    static textio_pl011_device_t pl011_device = {0};

    // get driver fwk threadx handle
    PDFWK_THREADX_HOST drvfwk = (PDFWK_THREADX_HOST)fpfw_init_get_handle(dfwk_id);

    // Initialize the pl011 uart device.
    textio_pl011_device_initialize(&pl011_device, &pl011_config, &drvfwk->Schedule);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &pl011_device};
}

FPFW_INIT_COMPONENT(mctp, FPFW_INIT_DEPENDENCIES("mctp_uart"))
{
    textio_pl011_device_t* pl011_dev = (textio_pl011_device_t*)fpfw_init_get_handle("mctp_uart");
    if (pl011_dev == NULL)
    {
        // Skip if there is no mctp_uart device.
        return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
    }

    static fpfw_mctp mctp_context;
    static fpfw_mctp_uart_binding_ctx mctp_uart_binding_ctx = {0};
    static textio_pl011_interface_t pl011_interface;
    static uint8_t s_msg_types[] = {MSG_TYPE_PLDM};
    static uint32_t s_msg_versions[] = {PLDM_VERSION};

    textio_pl011_device_interface_initialize(pl011_dev, &pl011_interface);

    static fpfw_mctp_uart_packet_info mctp_uart_packets[NUM_MCTP_PACKETS] = {0};
    static fpfw_mctp_uart_binding_config mctp_uart_binding_config = {.name = "mctp_uart_binding",
                                                                     .uart_interface = &pl011_interface.header,
                                                                     .mctp_ctx = &mctp_context,
                                                                     .packet_array = mctp_uart_packets,
                                                                     .num_packets = NUM_MCTP_PACKETS,
                                                                     .num_pending_rx_requests = NUM_RX_MCTP_PACKETS};
    fpfw_status_t status = fpfw_mctp_uart_binding_init(&mctp_uart_binding_ctx, &mctp_uart_binding_config);
    if (FPFW_STATUS_FAILED(status))
    {
        return (fpfw_init_result_t){status, NULL};
    }

    static fpfw_mctp_config mctp_config = {.mctp_instance_name = "MCTP0",
                                           .message_support_len = NUM_MSG_TYPES,
                                           .binding = {.mctp_binding_ctx = &mctp_uart_binding_ctx,
                                                       .send_pkt = fpfw_mctp_uart_binding_send_pkt,
                                                       .get_pkt = fpfw_mctp_uart_binding_get_pkt,
                                                       .release_pkt = fpfw_mctp_uart_binding_release_pkt},
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
    FPFW_DBGPRINT_ALWAYS("PLDM platform event ready callback called with event ID: %u", event_id);
}

// Supported PLDM message types
static uint32_t s_supported_msg_types[] = {FPFW_PLDM_TYPE_PLATFORM_MONITORING_CONTROL};
// Number of supported PLDM message types
static size_t s_supported_msg_types_count = sizeof(s_supported_msg_types) / sizeof(s_supported_msg_types[0]);

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
                                               .auto_discovery_notify = true};

    pldmConfig.mctp_ctx = (fpfw_mctp*)fpfw_init_get_handle("mctp");
    pldmConfig.pdr_repo = (void*)fpfw_init_get_handle("pdr_repo");
    pldmConfig.supported_msg_types = s_supported_msg_types; // Supported message types
    pldmConfig.supported_msg_types_count = s_supported_msg_types_count;

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
