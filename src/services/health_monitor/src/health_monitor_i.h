//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_i.h
 * 
 * Private header file for supporting health monitoring
 */
#pragma once

/*------------- Includes -----------------*/
#include <health_monitor.h>

/*-- Symbolic Constant Macros (defines) --*/
#define HM_MODULE_NAME "[HM_SVC] "
#ifndef NEWLINE
#define NEWLINE "\n"
#endif

#define HM_LOG_INFO(fmt, ...) printf(HM_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define HM_LOG_WARN(fmt, ...) printf(HM_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define HM_LOG_CRIT(fmt, ...) printf(HM_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

#define MAX_CPER_CACHE 4

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void activate_error_domain(uint16_t error_domain_idx, const guid_t *p_error_domain_guid, const char *p_error_domain_txt);
void construct_mscp_ghes_table();
void set_ghes_table_ready();
bool ddr_subsystem_enabled();
void hm_register_cached_error_domain();
void hm_submit_cached_cper();
void hm_report_uncorrected_error(hm_error_report_type_t type);
void update_error_record_section(uint16_t error_domain_idx, acpi_error_severity_t err_severity, void *err_record_section, uint32_t err_record_section_size);
void hm_prepare_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_inject_error_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status);
hm_error_domain_info_t* hm_get_registered_error_domain(acpi_error_domain_t error_domain_idx);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
