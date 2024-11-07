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
#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/*-------------------------------- Typedefs ---------------------------------*/

/*-- Declarations (Statics and globals) --*/
static var_service_req_ctx_t var_svc_ctx = {};

/*-------------- Functions ---------------*/
void hsp_variable_service_initialize(var_service_shared_mem_t* var_svc_mem_ctx)
{
    variable_service_initialize_ctx(&var_svc_ctx, var_svc_mem_ctx);
}

void read_knob_from_hsp(cached_knob_data_t* current_entry)
{
    var_service_req_params_t var_svc_req = {};
    var_svc_req.variable_name_ptr = (uint16_t*)current_entry->name;
    var_svc_req.variable_name_size = strlen(current_entry->name) + 1;
    memcpy(&var_svc_req.vendor_namespace_guid, current_entry->knob_namespace, sizeof(var_svc_req.vendor_namespace_guid));
    var_svc_req.data_size = current_entry->size;
    var_svc_req.data = current_entry->data;

    int32_t result = variable_service_sync_get_variable(&var_svc_ctx, &var_svc_req);

    if (KNG_SUCCEEDED(result))
    {
        current_entry->overridden = true;
    }

    variable_service_unlock_get_var_ctx(&var_svc_ctx);
}

void write_knob_to_hsp(cached_knob_data_t* current_entry)
{
    var_service_req_params_t var_svc_req = {};
    var_svc_req.variable_name_ptr = (uint16_t*)current_entry->name;
    var_svc_req.variable_name_size = strlen(current_entry->name) + 1;
    memcpy(&var_svc_req.vendor_namespace_guid, current_entry->knob_namespace, sizeof(var_svc_req.vendor_namespace_guid));
    var_svc_req.data_size = current_entry->size;
    var_svc_req.data = current_entry->data;
    var_svc_req.attributes.as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    variable_service_sync_set_variable(&var_svc_ctx, &var_svc_req);
}