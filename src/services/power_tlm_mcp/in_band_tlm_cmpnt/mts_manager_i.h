//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mts_manager_i.h
 * Manage data collection service interaction.
 */

#pragma once

/*----------- Nested includes ------------*/
#include "ddr_manager_i.h"

#include <FpFwAssert.h>
#include <FpFwLinkedList.h>
#include <exec_tlm_cmpnt.h>
#include <fpfw_status.h>
#include <pwr_tlm_core_exchange.h>
#include <stdint.h>
#include <transfer_relay_protocol.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define TLM_CLIENT_SET_MODE_MSG_SIZE (4)

#define MEMBER_SIZE(type, member) (sizeof( ((type *)0)->member ))

/*-------------- Typedefs ----------------*/

typedef struct
{
   FPFW_LIST_ENTRY list_entry;
   trp_msg_read_pkg_rsp_t pkg;
} tlm_package_t, *p_tlm_package_t;


/*-- Declarations (Statics and globals) --*/
extern tlm_package_t mts_active_pkg_buffer[MAX_PENDING_PACKAGES];

extern FPFW_LIST_HANDLE pkg_free_list;

// list usage depends on which die tlm service is running on.
// On Die 0 MCP, this list will contain active packages from local as well as secondary MCP's.
// On Die 1 MCP, this list will contain active packages from local MCP only. Packages are immediately sent
// do Die 0, but the list is maintained on the local MCP to clear all packages on a DCP reset command.
extern FPFW_LIST_HANDLE pkg_active_list;

extern p_tlm_package_t in_flight_tlm_pkg;

extern bool mpam_vm_mem_reporting_knob_enable;

/*--------- Function Prototypes ----------*/

// *************************************************************************************************
// ** Public API's ** /
// *************************************************************************************************

/**
 * @brief Initialize the data collection service manager.
 * @param[in] mpam_vm_mem_enable True to enable MPAM VM memory power reporting, false to disable.
 */
void mts_manager_init(bool mpam_vm_mem_enable);

/**
 * @brief Queue a telemetry package for eventual transmission to the host.
 *
 * @param[in] atu_mapped_location - location of the package in local atu mapped address space
 * @param[in] pkg_size - size of the package
 */
void mts_manager_queue_tlm_package(uintptr_t atu_mapped_location, size_t pkg_size);

// *************************************************************************************************
// ** Private API's ** /
// *************************************************************************************************

/**
 * @brief Add a telemetry package to the active list.
 *
 * @param[in] tlm_pkg - Pointer to the package to dd
 */
 void mts_manager_add_tlm_package_to_active_list(p_tlm_package_t tlm_pkg);

/**
 * @brief Handle a DCP message from the data collection service.
 *
 * @param[in] trp_msg - Pointer to the incoming TRP message
 */
void mts_manager_handle_dcp_msg(p_trp_msg_t trp_msg);

/**
 * @brief Handle a TRP message from the data collection service.
 *
 * @param[in] trp_msg - Pointer to the incoming TRP message
 */
void mts_manager_handle_trp_msg(p_trp_msg_t trp_msg);

/**
 * @brief Handle a record enable/disable message from the data collection service.
 *
 * @param[in,out] trp_msg - Pointer to the incoming TRP message
 */
void mts_manager_handle_record_enable_disable(p_trp_msg_t trp_msg);

/**
 * @brief Handle a read message from the data collection service.
 *
 * @param[in,out] tlm_pkg - Pointer to the telemetry package
 */
void mts_manager_handle_read_msg(p_trp_msg_t trp_msg);

/**
 * @brief Handle a read complete message from the data collection service.
 *
 * @param[in,out] tlm_pkg - Pointer to the telemetry package
 */
void mts_manager_handle_read_complete_msg(p_trp_msg_t trp_msg);

/**
 * @brief Send a TRP package notification to the primary MCP.
 *
 * @param[in] tlm_pkg - Pointer to the telemetry package
 */
void mts_manager_send_trp_pkg_notification_to_primary(p_tlm_package_t tlm_pkg);

/**
 * @brief Send a TRP read complete message to the source of the package.
 *
 * @param[in] tlm_pkg - Pointer to the telemetry package
 */
void mts_manager_send_trp_read_complete(p_tlm_package_t tlm_pkg);

/**
 * @brief Free a telemetry package. Only called on primary MCP. If the package is local, the memory it is pointing to is deallocated.
 * If the package is remote, a TRP message is sent for the remote to deallocate.
 *
 * @param[in] tlm_pkg - Pointer to the telemetry package
 */
void mts_manager_free_tlm_package_from_primary_mcp(p_tlm_package_t tlm_pkg);

/**
 * @brief Free a telemetry package. Only called on secondary MCP. Free's local memory.
 *
 * @param[in] tlm_pkg - Pointer to the telemetry package
 */
void mts_manager_free_tlm_package_from_secondary_mcp(p_tlm_package_t tlm_pkg);

/**
 * @brief Get a telemetry package from the free list.
 *
 * @return Pointer to the telemetry package. NULL if no packages are available.
 */
p_tlm_package_t mts_manager_get_pkg_from_free_list(void);

/**
 * @brief Send a TRP package message to the specified destination.
 *
 * @param[in] tlm_pkg - Pointer to the telemetry package
 * @param[in] msg_id - TRP message ID. Valid messages are TRP_MSG_ID_PACKAGE_NOTIFICATION, TRP_MSG_ID_READ_PACKAGE_RESPONSE, TRP_MSG_ID_READ_PACKAGE_COMPLETE
 * @param[in] dest_die_id - Destination die ID
 * @param[in] dest_core_id - Destination Core ID
 */
void mts_manager_send_trp_package_helper(p_tlm_package_t tlm_pkg, trp_msg_id_t msg_id, uint8_t dest_die_id, uint8_t dest_core_id);

/**
 * @brief stops publishing telemetry data and frees all resources.
 *
 */
void mts_manager_free_publish_resources(void);

/**
 * @brief Initialize the TRP header for a new telemetry message.
 *
 * @param[in,out] trp_msg - Pointer to the new TRP message
 * @param[in] payload_size - Size of the payload in bytes
 */
void mts_manager_init_trp_header_for_broadcast(p_trp_msg_t trp_msg, uint16_t payload_size);

/**
 * @brief Changes the telemetry service mode and notifies secondary cores if called from
 * the primary core.
 * @param[in] new_mode The new telemetry mode.
 * @return None
 *
 */
void mts_manager_send_mode_to_sec_cores(tlm_operating_mode_t new_mode);

/**
 * @brief Notify the secondary MCPS to prepare for a power package.
 *
 *
 * @return None
 */
void mts_manager_send_prep_pwr_pkg_notification_to_sec_mcps(void);

/**
 * @brief Notify the SCP to prepare for a power package.
 *
 *
 * @return None
 */
void mts_manager_send_prep_pwr_pkg_notification_to_scp(void);

/**
 * @brief Send the record enables to the SCP.
 *
 * @param[in] enables - The record enables to send.
 */
void mts_manager_send_record_enables_to_scp(p_tlm_scp_record_enables_t enables);

