//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file cfg_mgr_init.c
 * This file contains the config/initialization for the fpfw configuration manager.
 */

/*------------- Includes -----------------*/

#include <FpFwUtils.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_cfg_mgr_init.h>
#include <fpfw_init.h>         // for fpfw_init_get_handle, FPFW_INIT_STATU...
#include <stddef.h>            // for NULL
#include <stdio.h>             // for printf, NULL

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/
bool empty_read_knob_fn(const fpfw_cfg_mgr_guid_t* knob_namespace, const char* knob_name, uint8_t* data, size_t data_size, void* ctx)
{
    FPFW_UNUSED(knob_namespace);
    FPFW_UNUSED(knob_name);
    FPFW_UNUSED(data);
    FPFW_UNUSED(data_size);
    FPFW_UNUSED(ctx);

    return true;
}

bool empty_write_knob_fn(const fpfw_cfg_mgr_guid_t* knob_namespace, const char* knob_name, const uint8_t* data, size_t data_size, void* ctx)
{
    FPFW_UNUSED(knob_namespace);
    FPFW_UNUSED(knob_name);
    FPFW_UNUSED(data);
    FPFW_UNUSED(data_size);
    FPFW_UNUSED(ctx);

    return true;
}

FPFW_INIT_COMPONENT(cfg_mgr, FPFW_INIT_DEPENDENCIES("std_io", "dfwk", "hw_ver"))
{
    // This struct is only used to initialize a fpfw_cfg_mgr_db_t struct
    fpfw_cfg_mgr_config_t cfg = {
        .mission_mode = false,
        .profile_id = 0,
        .read_knob_fn = empty_read_knob_fn,
        .write_knob_fn = empty_write_knob_fn,
        .db_ctx = (void*)1
    };

    fpfw_cfg_mgr_init(&cfg);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, NULL};
}
