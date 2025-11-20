//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file pwr_tlm_core_exchange.h
 * This file contains the interface for the data exchange between SCP and MCP for power telemetry
 * This library is setup for single writer, single reader. Communication synchronization is done outside of this
 * library.  This lib only provides cache management for the interface.
 *
 * +----------------+                          +----------------+                                +----------------+
 * |      MCP       |                          |     Exchange   |                                |      SCP       |
 * +----------------+                          +----------------+                                +----------------+
 *       |                                             |                                                 |
 *       |  Notify 2min Pkg Prep                       |                                                 |
 *       |---------------------------------------------|------------------------------------------------>|
 *       |                                             |                                                 |
 *       |                                             |  pwr_tlm_core_exch_scp_write_droop_counts()     |
 *       |                                             |<------------------------------------------------|
 *       |                                             |                                                 |
 *       |       ... Prep to Package Delay ...         |                                                 |
 *       |                                             |                                                 |
 *       |  pwr_tlm_core_exch_mcp_read_droop_counts()  |                                                 |
 *       |-------------------------------------------->|                                                 |
 *       |<--------------------------------------------|                                                 |
 *       |                                             |                                                 |
 *       |-----------                                  |                                                 |
 *       |          | Create 2min Pkg                  |                                                 |
 *       |<---------                                   |                                                 |
 *       |                                             |                                                 |
 *
 *  */

#pragma once

/*----------- Nested includes ------------*/
#include <idsw_kng.h>
#include <kng_soc_constants.h>
#include <stdint.h>



/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef union
{
    struct {
        uint16_t drop_count_en:1;
        uint16_t vm_memory_pwr_en:1;
        uint16_t reserved:14;
    }record;
    uint16_t as_uint16;
}tlm_scp_record_enables_t, *p_tlm_scp_record_enables_t;

typedef enum
{
   TLM_CLIENT_CMD_SET_MODE_PUSH = 1,
   TLM_CLIENT_CMD_GEN_PWR_PACKAGE_MCP_2_SCP_PUSH = 2,
   TLM_CLIENT_CMD_GEN_PWR_PACKAGE_PRIM_MCP_2_SEC_MCP_PUSH = 3,
   TLM_CLIENT_CMD_GEN_24HR_PACKAGE_MCP_2_SCP_PUSH = 4,
   TLM_CLIENT_CMD_SYNC_REC_ENABLES_MCP_2_SCP_PUSH = 5,
} tlm_client_cmd_t;

typedef struct __attribute__((packed)) tlm_client_msg_t{
    uint16_t cmd;                                                   // tlm_client_cmd_t
    union __attribute__((packed)) {
        uint16_t                       mode;                        // tlm_operating_mode_t
        tlm_scp_record_enables_t       scp_records;
    } payload;
} tlm_client_msg_t, *p_tlm_client_msg_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize the core to core exchange structure. This function should be called once during system initialization.
 * Only call from SCP init.
 */
void pwr_tlm_core_exch_init(void);

/**
 * @brief Write the droop counts for the specified core. This function is intended to be called by the SCP.
 *
 * @param[in] droop_count_array Pointer to the array of droop counts to write.
 */
void pwr_tlm_core_exch_scp_write_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE]);

/**
 * @brief Read the droop counts for the specified core. This function is intended to be called by the MCP.
 *
 * @param[out] droop_count_array destination pointer to array where droop counts will be written.
 * @return sequence number of the droop counts. This is used to determine if the data is stale from last read
 */
uint8_t  pwr_tlm_core_exch_mcp_read_droop_counts(uint64_t (*droop_count_array)[NUM_AP_CORES_PER_DIE]);