/**
 * @file etr_init.c
 * Instantiates Event Tracing Relay for the MCP
 */

/*------------- Includes -----------------*/

#include <FpFwUtils.h>
#include <event_trace_relay.h>
#include <fpfw_init.h>
#include <in_band_telemetry_ddr.h>
#include <stddef.h>
#include <stdint.h>
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

FPFW_INIT_COMPONENT(etr, FPFW_INIT_DEPENDENCIES("etc"))
{

    etr_service_config_t config = {
        // @TODO: grab these from the idhw/idhw libraries or fuses once available on the MCP
        //        https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1743379
        .soc_info =
            {
                .die_id = 0,
                .soc_id = 0,
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
    };

    etr_initialize(&s_etr_service_ctx, &config);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &s_etr_service_ctx};
}
