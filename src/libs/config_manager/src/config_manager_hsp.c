//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file config_manager_hsp.c
 * This file contains the hsp communication for the configuration manager.
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

/*-------------------------------- Typedefs ---------------------------------*/
/*-- Declarations (Statics and globals) --*/
extern knob_data_t g_knob_data[];
static var_service_req_ctx_t var_svc_ctx = {};

/*-------------- Functions ---------------*/
void hsp_variable_service_initialize(var_service_shared_mem_t* var_svc_mem_ctx)
{
    variable_service_initialize_ctx(&var_svc_ctx, var_svc_mem_ctx);
}

static void check_var_store_cb(void* context, struct _variable_service_req_ctx* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size)
{
    cached_knob_data_t* current_entry = (cached_knob_data_t*)context;

    if (KNG_SUCCEEDED(var_serv_ctx->async_req_result))
    {
        if (current_entry->size <= data_size)
        {
            if (memcmp(data_start_ptr, current_entry->data, current_entry->size) == 0)
            {
                CFG_INFO("var store has same value, idx[%d]", current_entry->index);
            }
            else
            {
                CFG_INFO("var store value doesn't matched with current, idx[%d]", current_entry->index);
            }
        }
        else
        {
            CFG_CRIT("var store contains invalid data, idx[%d]", current_entry->index);
        }
    }
    else
    {
        // var store read operation failed
        CFG_CRIT("var store access failed, idx[%d]", current_entry->index);
    }

    variable_service_unlock_get_var_ctx(&var_svc_ctx);
}

void check_var_store_knob_data_async(cached_knob_data_t* current_entry)
{
    var_service_req_params_t var_svc_req = {};
    var_svc_req.variable_name_ptr = (uint16_t*)current_entry->name;
    var_svc_req.variable_name_size = strlen(current_entry->name) + 1;
    memcpy(&var_svc_req.vendor_namespace_guid, current_entry->knob_namespace, sizeof(var_svc_req.vendor_namespace_guid));
    var_svc_req.data_size = current_entry->size;
    var_svc_req.attributes_size = 0;

    int32_t result = variable_service_async_get_variable(&var_svc_ctx, &var_svc_req, check_var_store_cb, current_entry);
    BUG_ASSERT_PARAM(KNG_SUCCEEDED(result), result, KNG_SUCCESS);
}

static void write_knob_completion_routine(void* context,
                                          struct _variable_service_req_ctx* var_serv_ctx,
                                          uint8_t* data_start_ptr,
                                          size_t data_size)
{
    BUG_ASSERT_PARAM(KNG_SUCCEEDED(var_serv_ctx->async_req_result), var_serv_ctx->async_req_result, var_serv_ctx);

    var_store_ctx_t* written_item = (var_store_ctx_t*)context;
    CFG_INFO("new knob value was set, [%s]\n", written_item->knob_item->name);

    if (written_item->cb != NULL)
    {
        written_item->cb(written_item->knob_item, data_start_ptr, data_size);
    }
}

void read_knob_from_hsp(cached_knob_data_t* current_entry)
{
    // Retrieve the knob from HSP during init phase. make sure we update knobs before init phase completed.
    uint32_t var_name_length = strlen(current_entry->name);
    uint16_t var_wide_name[var_name_length + 1];

    for (uint32_t i = 0; i < var_name_length; ++i)
    {
        var_wide_name[i] = (uint16_t)current_entry->name[i];
    }
    var_wide_name[var_name_length] = 0x00;

    var_service_req_params_t var_svc_req = {};
    var_svc_req.variable_name_ptr = var_wide_name;
    var_svc_req.variable_name_size = (var_name_length + 1) * sizeof(uint16_t);
    memcpy(&var_svc_req.vendor_namespace_guid, current_entry->knob_namespace, sizeof(var_svc_req.vendor_namespace_guid));
    var_svc_req.data_size = current_entry->size;
    var_svc_req.data = current_entry->data;

    int32_t result = variable_service_sync_get_variable(&var_svc_ctx, &var_svc_req);

    if (KNG_SUCCEEDED(result))
    {
        if (memcmp(current_entry->data, g_knob_data[current_entry->index].default_value_address, current_entry->size) != 0)
        {
            CFG_INFO("knob value has been overridden");
            current_entry->overridden = true;
        }
        else
        {
            current_entry->overridden = false;
        }
    }
    else
    {
        current_entry->overridden = false;
    }

    variable_service_unlock_get_var_ctx(&var_svc_ctx);
}

void write_knob_to_hsp(cached_knob_data_t* current_entry, update_knob_completion_routine cb)
{
    static var_store_ctx_t var_ctx = {0};
    var_ctx.knob_item = current_entry;
    var_ctx.cb = cb;

    uint32_t var_name_length = strlen(current_entry->name);
    uint16_t var_wide_name[var_name_length + 1];

    for (uint32_t i = 0; i < var_name_length; ++i)
    {
        var_wide_name[i] = (uint16_t)current_entry->name[i];
    }
    var_wide_name[var_name_length] = 0x00;

    var_service_req_params_t var_svc_req = {};
    var_svc_req.variable_name_ptr = var_wide_name;
    var_svc_req.variable_name_size = (var_name_length + 1) * sizeof(uint16_t);
    memcpy(&var_svc_req.vendor_namespace_guid, current_entry->knob_namespace, sizeof(var_svc_req.vendor_namespace_guid));
    var_svc_req.data_size = current_entry->size;
    var_svc_req.data = current_entry->data;
    var_svc_req.attributes.as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    int32_t result =
        variable_service_async_set_variable(&var_svc_ctx, &var_svc_req, write_knob_completion_routine, &var_ctx);
    BUG_ASSERT_PARAM(KNG_SUCCEEDED(result), result, KNG_SUCCESS);
}