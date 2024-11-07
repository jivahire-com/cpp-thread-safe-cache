//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file config_manager.h
 *  Configuration Manager Library for KNG SOC 
 */

#pragma once

/*--------------- Includes ---------------*/
#include <fpfw_cfg_mgr.h>
#include <fpfw_cfg_mgr_init.h>
#include <variable_services.h>

/*-------------- Typedefs ----------------*/
typedef struct _cached_knob_data_t{
    knob_t index;
    const fpfw_cfg_mgr_guid_t* knob_namespace;
    const char* name;
    void* data;
    size_t size;
    bool overridden;
} cached_knob_data_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief Initializes the config manager.
 * @param cfg_mgr_config
 *     Configuration for the config manager.
 * @param var_svc_mem_ctx
 *    Pointer to the shared memory context for the variable service.
 */
void cfg_mgr_init(fpfw_cfg_mgr_config_t* cfg_mgr_config, var_service_shared_mem_t* var_svc_mem_ctx);

/**
 * @brief 
 *  Read a knob value from a default configuration database
 * 
 * @param knob_namespace
 *  Pointer to the 16 byte guid matching the desired knob's namespace
 *  
 * @param knob_name 
 *  Pointer to the string representing the knob name, c-style NULL terminated ASCII encoded.
 * 
 * @param data 
 *  Pointer to memory to store the knob value
 * 
 * @param data_size 
 *  Size, in bytes, of data
 * 
 * @param ctx 
 *  Pointer to context setup by the interface
*/

bool read_knob_from_default_db_cb(const fpfw_cfg_mgr_guid_t* knob_namespace, const char* knob_name, uint8_t* data, size_t data_size, void* ctx);

/**
 * @brief 
 *  Update a knob value in the cached configuration database
 * 
 * @param knob_namespace
 *  Pointer to the 16 byte guid matching the desired knob's namespace
 *  
 * @param knob_name 
 *  Pointer to the string representing the knob name, c-style NULL terminated ASCII encoded.
 * 
 * @param data 
 *  Pointer to memory containing the knob value
 * 
 * @param data_size 
 *  Size, in bytes, of data
 * 
 * @param ctx 
 *  Pointer to context setup by the interface
*/
bool update_knob_in_cached_db_cb(const fpfw_cfg_mgr_guid_t* knob_namespace, const char* knob_name, const uint8_t* data, size_t data_size, void* ctx);

/**
 * @brief 
 *  return size of cached knob data
 *
*/
uint32_t cached_knob_data_size(void);

/**
 * @brief 
 *  return cached knob data
 *
*/
cached_knob_data_t* get_cached_knob_data(void);

/**
 * @brief   
 *  Update the knob data
 * 
 * @param current_entry 
 *  Pointer to the knob data which needs to be updated
 * @param data 
 *  Pointer to the new data
 * @param data_size  
 *  Size of the new data
 * @param permanent
 *  Flag to indicate if the data update is permanent
 */
bool update_knob_data(cached_knob_data_t* current_entry, const uint8_t* data, size_t data_size, bool permanent);