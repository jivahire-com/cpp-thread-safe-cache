/**
 * @file etr_init.c
 * Instantiates Event Tracing Relay for the MCP
 */

/*------------- Includes -----------------*/
#include <FpFwUtils.h>
#include <cli_et_telemetry.h>
#include <event_trace_relay.h>
#include <fpfw_init.h>
#include <idsw.h>
#include <string.h>
#include <tlm_fuses.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

// Thread Stack Sizes
#define ETR_STACK_SIZE ((TX_MINIMUM_STACK) + ((4) * (FPFW_KB)))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static etr_service_context_t s_etr_service_ctx = {0};
static uint8_t s_etr_stack[ETR_STACK_SIZE];

/*------------- Functions ----------------*/

FPFW_INIT_COMPONENT(etr, FPFW_INIT_DEPENDENCIES("etc", "icc_hspmbx", "atu_svc", "hw_ver", "mts_svc", "tlm_fuses"))
{

    etr_service_config_t config = {
        .soc_info =
            {
                .die_id = idsw_get_die_id(),
            },
        .asic_ddr_config =
            {
                .base_addr = IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_BASE_ADDR,
                .size_bytes = IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE,
            },
        .hsp_ddr_config =
            {
                .base_addr = IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_BASE_ADDR,
                .size_bytes = IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE,
            },
        .thread_config =
            {
                .p_stack = s_etr_stack,
                .stack_size = sizeof(s_etr_stack),
                .priority = 10,
                .time_slice_option = TX_NO_TIME_SLICE,
            },
        .icc_config =
            {
                .p_hsp_icc_ctx = fpfw_init_get_handle("icc_hspmbx"),
            },
    };

    // Get the ECID and format it into the soc_id array for the ASIC Header
    ecid_t ecid = {0};
    tlm_fuses_get_ecid(&ecid);

    // Format soc_id according to the specified bit layout
    // soc_id is 16 bytes (128 bits), all initialized to 0
    // Parity Bits (5 bits):       bits 0-4
    // Y Coordinate (7 bits):      bits 5-11
    // X Coordinate (7 bits):      bits 12-18
    // Wafer Number (5 bits):      bits 19-23
    // Wafer Lot Number (72 bits): bits 24-95
    // Reserved (32 bits):         bits 96-127
    memset(config.soc_info.soc_id, 0, sizeof(config.soc_info.soc_id));

    // Ensure non-overlapping bits for Parity Bits and Y coordinate in soc_id[0]
    uint8_t parity_bits_byte_0 = (ecid.parity_bits & 0x1F);   // bits 0-4
    uint8_t y_coord_bits_byte_0 = (ecid.y_coord & 0x7F) << 5; // bits 5-7

    // Parity Bit (5 bits) + Y Coordinate (first 3 bits)
    config.soc_info.soc_id[0] = parity_bits_byte_0 | y_coord_bits_byte_0;

    // Ensure non-overlapping bits for Y and X coordinates in soc_id[1]
    uint8_t y_coord_bits_byte_1 = ((ecid.y_coord & 0x7F) >> 3) & 0x0F; // bits 8-11
    uint8_t x_coord_bits_byte_1 = ((ecid.x_coord & 0x7F) << 4) & 0xF0; // bits 12-15

    // Y Coordinate (last 4 bits) + X Coordinate (first 4 bits)
    config.soc_info.soc_id[1] = y_coord_bits_byte_1 | x_coord_bits_byte_1;

    // Ensure non-overlapping bits for X coordinate and Wafer Number in soc_id[2]
    uint8_t x_coord_bits_byte_2 = (ecid.x_coord & 0x7F) << 0;     // bits 15-18
    uint8_t wafer_num_bits_byte_2 = (ecid.wafer_num & 0x1F) << 3; // bits 19-23

    // X Coordinate (last 3 bits) + Wafer Number (5 bits)
    config.soc_info.soc_id[2] = x_coord_bits_byte_2 | wafer_num_bits_byte_2;

    // Wafer Lot Number (72 bits)
    memcpy(&config.soc_info.soc_id[3], ecid.wafer_lot_num, ECID_WAFER_LOT_NUMBER_CHAR_SIZE); // bits 24-95

    etr_initialize(&s_etr_service_ctx, &config);

    // Initialize the CLI for Event Trace Telemetry
    evt_telemetry_cli_init();

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_etr_service_ctx};
}
