//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file config_manager.c
 * This file contains the config/initialization for the fpfw configuration manager.
 */

/*------------- Includes -----------------*/
#include <FpFwSpinLock.h>
#include <FpFwUtils.h>
#include <bug_check.h>
#include <config_manager.h>
#include <config_manager_i.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <system_info.h>

/*-------------------------------- Typedefs ---------------------------------*/

/*-- Declarations (Statics and globals) --*/
static cached_knob_data_t system_knob_cached[KNOB_MAX] = {0};
static FPFW_SPINLOCK lock;
static uint32_t knob_index = 0;

/*-------------- Functions ---------------*/
bool read_knob_from_default_db_cb(const fpfw_cfg_mgr_guid_t* knob_namespace,
                                  const char* knob_name,
                                  uint8_t* data,
                                  size_t data_size,
                                  void* ctx)
{
    FPFW_UNUSED(ctx);

    if (knob_index >= KNOB_MAX)
    {
        return false;
    }

    FPFwSpinLockAcquire(&lock);
    system_knob_cached[knob_index].index = (knob_t)knob_index;
    system_knob_cached[knob_index].knob_namespace = knob_namespace;
    system_knob_cached[knob_index].name = knob_name;
    system_knob_cached[knob_index].data = data;
    system_knob_cached[knob_index].size = data_size;
    system_knob_cached[knob_index].overridden = false;
    knob_index++;
    FPFwSpinLockRelease(&lock);

    return true;
}

bool update_knob_data(cached_knob_data_t* current_entry, const uint8_t* data, size_t data_size, bool permanent)
{
    if (data_size != current_entry->size)
    {
        return false;
    }

    FPFwSpinLockAcquire(&lock);
    memcpy(current_entry->data, data, data_size);
    current_entry->overridden = true;
    FPFwSpinLockRelease(&lock);

    if (permanent)
    {
        if (system_info_is_hsp_present())
        {
            write_knob_to_hsp(current_entry);
        }
        else
        {
            return false;
        }
    }

    return true;
}

bool update_knob_in_cached_db_cb(const fpfw_cfg_mgr_guid_t* knob_namespace,
                                 const char* knob_name,
                                 const uint8_t* data,
                                 size_t data_size,
                                 void* ctx)
{
    FPFW_UNUSED(knob_namespace);
    FPFW_UNUSED(ctx);

    for (uint32_t idx = 0; idx < knob_index; idx++)
    {
        cached_knob_data_t* current_entry = &system_knob_cached[idx];

        if (0 == strcmp(current_entry->name, knob_name))
        {
            return update_knob_data(current_entry, data, data_size, false);
        }
    }

    return true;
}

void apply_override_knob_from_hsp()
{
    for (uint32_t idx = 0; idx < cached_knob_data_size(); idx++)
    {
        FPFwSpinLockAcquire(&lock);
        read_knob_from_hsp(&(get_cached_knob_data()[idx]));
        FPFwSpinLockRelease(&lock);
    }
}

void cfg_mgr_init(fpfw_cfg_mgr_config_t* cfg_mgr_config, var_service_shared_mem_t* var_svc_mem_ctx)
{
    knob_index = 0;
    FPFwSpinLockInitialize(&lock);

    fpfw_cfg_mgr_init(cfg_mgr_config);

    // check all knob data properly loaded
    BUG_ASSERT_PARAM(knob_index == KNOB_MAX, knob_index, KNOB_MAX);

    if (cfg_mgr_config->mission_mode == false && system_info_is_hsp_present())
    {
        hsp_variable_service_initialize(var_svc_mem_ctx);
        apply_override_knob_from_hsp();
    }
}

uint32_t cached_knob_data_size(void)
{
    return knob_index;
}

cached_knob_data_t* get_cached_knob_data(void)
{
    return system_knob_cached;
}