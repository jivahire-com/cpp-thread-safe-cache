//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_service_scp_i.h
 * This file contains the private interface for scp pwr tlm svc
 */

#pragma once

/*----------- Nested includes ------------*/
#include <pwr_tlm_core_exchange.h>
#include <transfer_relay_protocol.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern tlm_scp_record_enables_t pwr_tlm_scp_record_enables;

/*--------- Function Prototypes ----------*/
/**
 * @brief API to init scp mts manager and register with MTS service.
 * @return None
 */
void mts_manager_scp_init(void);

/**
 * @brief Notify the scp telemetry service  that a new in-band MTS message has been received.
 * @param[in]  trp_msg
 * @return None
 */
void mts_manager_scp_handle_trp_msg(p_trp_msg_t trp_msg);

/**
 * @brief Handle a TRP message from the MTS service, this API is running on the callback from the MTS thread
 *
 * @param[in] trp_msg - Pointer to the incoming TRP message
 */
void pwr_tlm_scp_handle_incoming_mts_msgs(void);

/**
 * @brief API to prepare the droop count for pwr package request.
 * @return None
 */
void data_proc_scp_tlm_cmpnt_received_prep_droop_count_from_mcp(void);

/**
 * @brief API to prepare the VM memory power data received from MCP.
 * @return None
 */
void data_proc_scp_tlm_cmpnt_received_prep_vm_mem_pwr_from_mcp(void);

/**
 * @brief Handle the record enables sent from MCP to SCP.
 *
 * @param[in] enables - The record enables from MCP
 */
void data_proc_scp_tlm_cmpnt_handle_enables_from_mcp(tlm_scp_record_enables_t enables);

/**
 * @brief Initialize and write core Vmin values to core exchange.
 * This should be called once during SCP boot to populate Vmin table for MCP.
 * @return None
 */
void data_proc_scp_tlm_cmpnt_init_core_vmin(void);