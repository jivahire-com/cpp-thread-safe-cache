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
void read_knob_from_hsp(cached_knob_data_t* current_entry);

/**
 * @brief Writes a knob value to the HSP
 * @param current_entry
 *    Pointer to the cached knob data
 */
void write_knob_to_hsp(cached_knob_data_t* current_entry);