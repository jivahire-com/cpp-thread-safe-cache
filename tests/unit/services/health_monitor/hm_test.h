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
#include <fpfw_icc_base.h>
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
#define SDM_PROC_FRU "SDM_PROC"
#define SDM_PROC_ERROR_DOMAIN_FRU_GUID_DEF                 \
    {                                                      \
        0x349606B4, 0x659A, 0x5060,                        \
        {                                                  \
            0xC2, 0x98, 0x43, 0x52, 0xDA, 0x5E, 0x66, 0xDC \
        }                                                  \
    }
#define CDED_SDM_PROC_FRU "CDED_PROC"
#define CDED_SDM_PROC_ERROR_DOMAIN_FRU_GUID_DEF            \
    {                                                      \
        0x359707B5, 0x669B, 0x5161,                        \
        {                                                  \
            0xC3, 0x99, 0x44, 0x53, 0xDB, 0x5F, 0x67, 0xDD \
        }                                                  \
    }
#define HSP_PROC_FRU "HSP_PROC"
#define HSP_PROC_ERROR_DOMAIN_FRU_GUID_DEF                 \
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
acpi_einj_cmd_status_t hm_error_injection_cb(ras_einj_info_t* payload, void* ctx);
void custom_fpfw_recv(fpfw_icc_base_recv_req_t* req);
