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
// CPER address information per accelerator (provided via large FIFO mailbox)
typedef struct
{
    uint32_t valid;
    uint32_t accel_id;
    uint32_t cper_file_offset;
    uint32_t cper_file_size;
    uint32_t magic_nr_offset;
    uint32_t reserved;
} hm_accel_cper_addr_info_t;

/*-------- Function Prototypes -----------*/
void activate_error_domain(uint16_t error_domain_idx, const guid_t *p_error_domain_guid, const char *p_error_domain_txt);
void construct_mscp_ghes_table();
void set_ghes_table_ready();
bool ddr_subsystem_enabled();
void hm_register_cached_error_domain();
void hm_submit_cached_cper();
void hm_report_error_event(hm_error_report_type_t type, bool trigger);
void update_error_record_section(uint16_t error_domain_idx, acpi_error_severity_t err_severity, acpi_cper_section_t *err_record_section, uint32_t err_record_section_size);
void hm_prepare_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_prepare_mscp_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_prepare_sdm_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_prepare_cded_sdm_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_prepare_hsp_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_prepare_ap_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_inject_error_recv_cb(void* context, size_t output_size_bytes, fpfw_status_t status);
hm_error_domain_info_t* hm_get_registered_error_domain(acpi_error_domain_t error_domain_idx);
bool hm_mcp_error_record_submit_listener(fpfw_icc_base_ctx_t* icc_ctx);
bool hm_mcp_error_domain_register_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_hsp_error_domain_register_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_hsp_error_record_submit_listener(fpfw_icc_base_ctx_t* icc_ctx);
void hm_apcore_error_injection_listener(fpfw_icc_base_ctx_t* icc_ctx);
const char* get_error_domain_name(acpi_error_domain_t domain);
void hm_transfer_cper_mcp2bmc();
void hm_transfer_cper_to_bmc_internal(bool is_ue);
void hm_cper_transfer_listener_from_scp(fpfw_icc_base_ctx_t* icc_ctx);
void hm_cper_transfer_listener_from_secondary_mcp(fpfw_icc_base_ctx_t* icc_ctx);
void create_full_mscp_cper_record(acpi_error_domain_t err_domain_idx,
    acpi_error_severity_t severity,
    acpi_cper_section_t* err_record_section,
    uint32_t err_record_section_size,
    acpi_cper_record_t* cper_record);
void hm_set_pldm_transfer_status(hm_pldm_transfer_status status, bool is_ue);
hm_pldm_transfer_status hm_get_pldm_transfer_status(bool is_ue);
bool hm_is_fatal_error(uint32_t err_severity);
void hm_copy_cper_record(volatile uint8_t* dest, const uint8_t* src, size_t size);
