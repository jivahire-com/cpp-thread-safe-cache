//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor.h
 * Public header file for supporting health monitoring
 */
#pragma once

/*------------- Includes -----------------*/
#include <cper.h>
#include <einj.h>
#include <fpfw_icc_base.h>
#include <icc_mhu.h>
#include <kng_error.h>
#include <semaphore_lib.h>

/*-- Symbolic Constant Macros (defines) --*/
#define HM_GHES_VERSION_V2 10
#define HM_ERROR_RECORD_COUNT 1
#define HM_ERROR_SECTION_COUNT 2

/*------------- Typedefs -----------------*/
typedef acpi_einj_cmd_status_t (*hm_error_injection_cb_t)(ras_einj_info_t *p_information_base, void *error_domain_context);

typedef enum
{
    HM_INTERCORE_LOCAL,
    HM_INTERCORE_APCORE,
    HM_INTERCORE_REMOTE,
    HM_INTERCORE_HSP,
    HM_INTERCORE_SDM,
    HM_INTERCORE_CDED,
    HM_INTERCORE_TYPE_MAX
} hm_intercore_type_t;

typedef enum
{
    HM_ERROR_REPORT_GPIO,
    HM_ERROR_REPORT_INTERRUPT,
    HM_ERROR_REPORT_VARSVC,
} hm_error_report_type_t;

typedef enum
{
    HM_PLDM_TRANSFER_STATUS_IDLE = 0,
    HM_PLDM_TRANSFER_STATUS_REQUESTED
} hm_pldm_transfer_status;

typedef struct {
    // GHES table related
    acpi_ghes_t* mscp_ghes_base;
    ras_einj_info_t* mscp_error_injection_addr_base;
    uint64_t* mscp_ghes_error_record_addr_base;
    uint64_t* mscp_ghes_error_record_addr_table_base;
    uint64_t* mscp_ghes_ack_addr_table_base;
    // ICC context for intercore communication
    fpfw_icc_base_ctx_t *icc_ctx[HM_INTERCORE_TYPE_MAX];
    // HSP ICC Payload related
    const uint8_t* mscp_hsp_ras_payload_base;
    // MSCP CPER record location on RMSS memory
    const uint8_t* mscp_full_cper_record_base;
    // Semaphore related
    SEMAPHORE_ID semaphore_id;
    uint32_t semaphore_key;
    // Multi-die related
    bool is_primary;
    // Curent CPU type
    bool is_mcp;
} hm_config_t;

typedef struct {
    uint16_t error_domain_idx;
    uint8_t valid_fru_id;
    uint8_t valid_fru_str;
    guid_t fru_id;
    char fru_text[ACPI_FRU_TEXT_LENGTH];
    hm_error_injection_cb_t injection_cb;
    void *err_inject_ctx;
    bool activated;
} hm_error_domain_info_t;

typedef struct {
    uint16_t error_domain_idx;
    uint16_t reserved;
    uint32_t section_size;
    uint32_t err_severity;
    acpi_cper_section_t cper_section;
} hm_error_record_t;

typedef struct _hm_cper_pldm_payload_t
{
    acpi_cper_record_t cper_record;
    uint32_t transfer_status;
} hm_cper_pldm_payload_t;

typedef struct _hm_arsm_cper_backup_t
{
    hm_cper_pldm_payload_t last_cper_record;
    hm_cper_pldm_payload_t last_ue_cper_record;
} hm_arsm_cper_backup_t;

/*-------- Function Prototypes -----------*/
hm_config_t* get_hm_config();
acpi_einj_cmd_status_t hm_inject_error(void);

void hm_register_error_domain(uint16_t error_domain_idx,
                              const guid_t* error_domain_guid,
                              const char* error_domain_name,
                              hm_error_injection_cb_t err_inject_cb,
                              void* err_inject_ctx);

void hm_submit_cper(uint16_t error_domain_idx,
                    acpi_error_severity_t err_severity,
                    acpi_cper_section_t* err_record_section,
                    uint32_t err_record_section_size);

void hm_submit_cper_cd_state(uint16_t error_domain_idx,
                    acpi_error_severity_t err_severity,
                    acpi_cper_section_t* err_record_section,
                    uint32_t err_record_section_size);

void hm_pre_ddr_init(hm_config_t* hm_config);
void hm_post_ddr_init();
void hm_post_intercore_init(hm_intercore_type_t intercore_type, fpfw_icc_base_ctx_t* icc_ctx);
void hm_map_error_injection_payload();
void hm_unmap_error_injection_payload();
void hm_set_pldm_ready_status();
uint64_t AP_GHES_ADDR(uint32_t mscp_addr);
uint32_t MSCP_GHES_ADDR(uint64_t ap_addr);

void hm_update_accel_fatal_cper_info(uint32_t accel_id, uint32_t cper_buffer_offset, uint32_t cper_magic_nr_offset);
bool hm_read_cper_magic_valid(uint32_t accel_id);
bool hm_collect_accel_fatal_cper(uint32_t accel_id);
void hm_send_accel_error_cper(uint32_t accel_id);
bool hm_allow_ras_reporting(void);

