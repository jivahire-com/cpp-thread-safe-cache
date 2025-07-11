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
#include <DbgPrint.h>
#include <health_monitor.h>

/*-- Symbolic Constant Macros (defines) --*/
#define HM_MODULE_NAME "[HM_SVC] "
#ifndef NEWLINE
#define NEWLINE "\n"
#endif

#define HM_LOG_INFO(fmt, ...) FPFW_DBGPRINT_INFO(HM_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define HM_LOG_DBG(fmt, ...) FPFW_DBGPRINT_VERBOSE(HM_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define HM_LOG_ERR(fmt, ...) FPFW_DBGPRINT_ERROR(HM_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define HM_LOG_CRIT(fmt, ...) FPFW_DBGPRINT_ERROR(HM_MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

#define MAX_CPER_CACHE 4

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void hm_report_error_event(hm_error_report_type_t type, bool trigger);
void update_error_record_section(uint16_t error_domain_idx, acpi_error_severity_t err_severity, acpi_cper_section_t *err_record_section, uint32_t err_record_section_size);
void create_full_mscp_cper_record(acpi_error_domain_t err_domain_idx,
    acpi_error_severity_t severity,
    acpi_cper_section_t* err_record_section,
    uint32_t err_record_section_size,
    acpi_cper_record_t* cper_record);
const char* get_error_domain_name(acpi_error_domain_t domain);

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
