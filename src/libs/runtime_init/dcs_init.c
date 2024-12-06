//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file dcs_init.c
 * Instantiates Data Collection Service
 */

/*------------- Includes -----------------*/
#include <FpFwAssert.h>
#include <FpFwUtils.h>
#include <data_collection_service.h>
#include <fpfw_init.h>
#include <icc_mhu.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kng_icc_shared.h>
#include <mscp_uefi_shared_ddrss.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DCS_STACK_SIZE ((TX_MINIMUM_STACK) + ((2) * (FPFW_KB)))

#define TRP_MAX_ROUTES    (2)
#define TRP_MAX_ENDPOINTS (2)

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static uint8_t s_dcs_stack[DCS_STACK_SIZE];
static uint8_t s_ap_icc_endpt_rx_buffer[ICC_MHU_DDR_PAYLOAD_SIZE]; // driver framework thread copies to this buffer
static uint8_t s_local_mscp_icc_endpt_rx_buffer[ICC_MHU_DDR_PAYLOAD_SIZE]; // driver framework thread copies to this buffer
static trp_icc_endpoint_t s_trp_icc_endpoint_table[TRP_MAX_ENDPOINTS];
static trp_route_t s_trp_routing_table[TRP_MAX_ROUTES];

/*------------- Functions ----------------*/
// icc_mhu_packet_t includes icc header
static_assert(sizeof(icc_mhu_packet_t) + sizeof(trp_msg_t) <= ICC_MHU_DDR_PAYLOAD_SIZE, "MHU payload size too small");

FPFW_INIT_COMPONENT(dcs_svc, FPFW_INIT_DEPENDENCIES("hw_ver", "atu_svc", "icc_mscp2mscp"))
{
    static dcs_config_t s_config = {
        .thread_config =
            {
                .p_stack = s_dcs_stack,
                .stack_size = sizeof(s_dcs_stack),
                .priority = 5,
                .time_slice_option = TX_NO_TIME_SLICE,
            },
        .trp_icc_config =
            {
                .routing_table = s_trp_routing_table,
                .number_of_routes = 0,
                .endpoint_table = s_trp_icc_endpoint_table,
                .number_of_endpoints = 0,
                //.this_die_id assigned below
                //.this_cpu_id assigned below
            },
    };

    uint16_t number_of_routes = 0;
    uint16_t number_of_endpoints = 0;

    s_config.trp_icc_config.this_die_id = idsw_get_die_id();
    s_config.trp_icc_config.this_cpu_id = idsw_get_cpu_type();

    // add AP dcp endpoint
    if ((s_config.trp_icc_config.this_die_id == DIE_0) && (s_config.trp_icc_config.this_cpu_id == CPU_MCP))
    {
        // TODO: initially using SCP for development, will be updated to use AP when available
        s_trp_icc_endpoint_table[number_of_endpoints].icc_base_ctx =
            (fpfw_icc_base_ctx_t*)fpfw_init_get_handle("icc_mscp2mscp");
        s_trp_icc_endpoint_table[number_of_endpoints].async_recv_buffer = s_ap_icc_endpt_rx_buffer;
        s_trp_icc_endpoint_table[number_of_endpoints].async_recv_buffer_size = sizeof(s_ap_icc_endpt_rx_buffer);
        s_trp_icc_endpoint_table[number_of_endpoints].icc_payload_protocol = ICC_COMMAND_DCP_MSG;
        sprintf(s_trp_icc_endpoint_table[number_of_endpoints].name, "%s", "DIE_0_AP_DCP"); // no sprintf_s, keep length to ENDPOINT_NAME_MAX_LENGTH

        s_trp_routing_table[number_of_routes].dest.die_id = DIE_0;
        s_trp_routing_table[number_of_routes].dest.cpu_id = CPU_AP;
        s_trp_routing_table[number_of_routes].icc_endpoint = &s_trp_icc_endpoint_table[number_of_endpoints];

        number_of_endpoints++;
        number_of_routes++;
    }

    // add local mscp trp endpoint
    FPFW_RUNTIME_ASSERT(number_of_routes < TRP_MAX_ROUTES);
    FPFW_RUNTIME_ASSERT(number_of_endpoints < TRP_MAX_ENDPOINTS);

    s_trp_icc_endpoint_table[number_of_endpoints].icc_base_ctx =
        (fpfw_icc_base_ctx_t*)fpfw_init_get_handle("icc_mscp2mscp");
    s_trp_icc_endpoint_table[number_of_endpoints].async_recv_buffer = s_local_mscp_icc_endpt_rx_buffer;
    s_trp_icc_endpoint_table[number_of_endpoints].async_recv_buffer_size = sizeof(s_local_mscp_icc_endpt_rx_buffer);
    s_trp_icc_endpoint_table[number_of_endpoints].icc_payload_protocol = ICC_COMMAND_TRP_MSG;
    // no sprintf_s, keep length to ENDPOINT_NAME_MAX_LENGTH
    sprintf(s_trp_icc_endpoint_table[number_of_endpoints].name,
            "DIE_%d_%s_TRP",
            s_config.trp_icc_config.this_die_id,
            (s_config.trp_icc_config.this_cpu_id == CPU_SCP) ? "MCP" : "SCP");

    s_trp_routing_table[number_of_routes].dest.die_id =
        s_config.trp_icc_config.this_die_id; // the icc context handle above is on the same die
    s_trp_routing_table[number_of_routes].dest.cpu_id =
        (s_config.trp_icc_config.this_cpu_id == CPU_SCP) ? CPU_MCP : CPU_SCP;
    s_trp_routing_table[number_of_routes].icc_endpoint = &s_trp_icc_endpoint_table[number_of_endpoints];

    number_of_endpoints++;
    number_of_routes++;

    s_config.trp_icc_config.number_of_routes = number_of_routes;
    s_config.trp_icc_config.number_of_endpoints = number_of_endpoints;

    dcs_init(&s_config);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}