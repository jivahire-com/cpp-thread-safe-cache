// Copyright Microsoft. All rights reserved.

/**
 * @file ap_core_i.h
 * This file contains the internal definitions for the APcore service
 */

#pragma once

/*----------- Nested includes ------------*/
#include "ap_core_init.h"

#include <DfwkCommon.h>
#include <FpFwAssert.h>
#include <FpFwLinkedList.h>
#include <fpfw_icc_base.h>          // for fpfw_icc_base_send, fpfw_icc_base...
#include <corebits.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
/*-- Symbolic Constant Macros (defines) --*/
#define MODULE_NAME "[APcore] "
#define NEWLINE     "\n"

// set to 1 for more verbose logs
#define APCORE_ENABLE_TRACE_LEVEL_LOG 1

#if APCORE_ENABLE_TRACE_LEVEL_LOG
#define APCORE_LOG_TRACE(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#else
#define APCORE_LOG_TRACE(fmt, ...) 
#endif
#define APCORE_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define APCORE_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define APCORE_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

/*-------------- Typedefs ----------------*/
typedef struct
{
    const ap_core_service_config_t* p_config;
    corebits_t enabled_cores;
    pap_core_asynchronous_request_t outstanding_request;
} ap_core_service_context_t, *pap_core_service_context_t;

typedef enum {
    AP_FW_ID_BL31 = 0,
    AP_FW_ID_STMM,
    AP_FW_ID_BL33,
    AP_FW_ID_HAFNIUM,
    AP_FW_ID_RMM,
    AP_FW_ID_SPMC,
    AP_FW_ID_MCP,
    AP_FW_ID_MAX,
} ap_fw_id_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

void ap_core_ppu_init(ap_core_service_context_t *p_context);
void ap_core_ppu_clusters_on(ap_core_service_context_t* p_context, uint32_t timeout_ms);
void ap_core_ppu_core_set_power_state(ap_core_service_context_t* p_context, unsigned core_idx, bool power_state_on, uint32_t timeout_ms);

unsigned int ap_core_util_boot_core(ap_core_service_context_t* p_context);
void ap_core_util_set_rvbaraddr(ap_core_service_context_t* p_context, unsigned core_idx, uint64_t rvbaraddr);
void ap_core_util_set_all_rvbaraddr(ap_core_service_context_t* p_context, uint64_t rvbaraddr);
void ap_core_util_get_fuse_enabled_cores(corebits_t *p_enabled_cores);
void ap_core_request_load_ap_fw(fpfw_icc_base_ctx_t* icc_hspmbx_ctx, ap_fw_id_t fw_id);
void ap_core_request_mcp_load(fpfw_icc_base_ctx_t* icc_hspmbx_ctx);
pap_core_asynchronous_request_t ap_core_get_outstanding_request();
