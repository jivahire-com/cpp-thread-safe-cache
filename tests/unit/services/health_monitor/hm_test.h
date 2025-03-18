/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file hm_test.h
 *
 */

#pragma once

/*----------- Includes ------------*/
#include <CMockaWrapper.h>
#include <health_monitor.h>
#include <health_monitor_i.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MCP_PROC_FRU "MCP_PROC"
#define MCP_PROC_ERROR_DOMAIN_FRU_GUID_DEF                 \
    {                                                      \
        0x339505B3, 0x649B, 0x4F5F,                        \
        {                                                  \
            0xC1, 0x97, 0x42, 0x51, 0xD9, 0x5D, 0x65, 0xDB \
        }                                                  \
    }
/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
int pre_ddr_setup(void** state);
int post_ddr_setup(void** state);
acpi_einj_cmd_status_t hm_error_injection_cb(ras_einj_info_t_temp* payload, void* ctx);
