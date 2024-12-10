//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file config_manager_i.h
 *  Configuration Manager Library for KNG SOC 
 */

#pragma once

/*--------------- Includes ---------------*/
#include <variable_services.h>

/*-------------- Typedefs ----------------*/
#define CFG_MGR_RELAY_PAYLOAD_SIGNATURE 0x6366

typedef struct _knob_payload_header_t {
    uint16_t signature;
    uint16_t total_override_knob_count;
} knob_payload_header_t;

typedef struct _knob_payload_t{
    uint16_t knob_index;
    uint16_t knob_data_size;
} knob_payload_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief Initializes the variable service for HSP communication
 * @param var_svc_mem_ctx
 *    Pointer to the shared memory context for the variable service.
 */
void hsp_variable_service_initialize(var_service_shared_mem_t* var_svc_mem_ctx);

/**
 * @brief Reads a knob value from the HSP
 * @param current_entry
 *    Pointer to the cached knob data
 */
void read_knob_from_hsp(cached_knob_data_t* current_entry, uint32_t current_knob_index);

/**
 * @brief Writes a knob value to the HSP
 * @param current_entry
 *    Pointer to the cached knob data
 */
void write_knob_to_hsp(cached_knob_data_t* current_entry);

/**
 * @brief Applies the override knob value from the HSP
 * @param rmss_base_addr
 *  Pointer to the base address of the shared RMSS memory
 * @param rmss_base_addr_size
 *  Size of the shared RMSS memory
 */
void write_override_knob_to_shared_rmss(void* rmss_base_addr, size_t rmss_base_addr_size);

/**
* @brief Applies the override knob value from the primary die
* @param rmss_base_addr
*  Pointer to the base address of the shared RMSS memory
*/
void apply_override_knob_from_primary_die(uint32_t rmss_base_addr);