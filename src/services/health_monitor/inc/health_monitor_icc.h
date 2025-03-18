//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_icc.h
 * Public header file for supporting health monitor icc communication
 */
#pragma once

/*------------- Includes -----------------*/
#include <cper.h>
#include <icc_mhu.h>  
#include <health_monitor.h>
#include <health_monitor_temporary_einj_structs.h>

/*-- Symbolic Constant Macros (defines) --*/
// this will go away once new silib is available
#ifndef ICC_GEN_CMD
#define ICC_GEN_CMD(module, cmd)         ((module << 16) + cmd)
#endif

#ifndef ICC_MODULE_HEALTH_MONITOR
#define ICC_MODULE_HEALTH_MONITOR        (0x0007)
#endif

#ifndef ICC_HM_ERROR_DOMAIN_REGISTER_MCP
#define ICC_HM_ERROR_DOMAIN_REGISTER_MCP    ICC_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, 0x1)
#endif

#ifndef ICC_HM_ERROR_RECORD_SUBMIT_MCP
#define ICC_HM_ERROR_RECORD_SUBMIT_MCP      ICC_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, 0x2)
#endif

#ifndef ICC_HM_ERROR_INJECTION_MCP
#define ICC_HM_ERROR_INJECTION_MCP          ICC_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, 0x3)
#endif

/*------------- Typedefs -----------------*/
typedef struct {
    icc_mhu_header_t header;
    uint16_t error_domain_idx;
    uint8_t valid_fru_id;
    uint8_t valid_fru_text;
    guid_t fru_id;
    char fru_text[ACPI_FRU_TEXT_LENGTH];
} hm_mhu_error_domain_register_payload_t;

typedef struct {
    icc_mhu_header_t header;
    hm_error_record_t error_record;
} hm_mhu_error_record_payload_t;

typedef struct {
    icc_mhu_header_t header;
    ras_einj_info_t_temp error_injection_info;
} hm_mhu_error_injection_payload_t;