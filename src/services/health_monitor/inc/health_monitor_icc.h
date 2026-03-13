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
#include <icc_platform_defines.h>
#include <icc_mhu.h>
#include <health_monitor.h>

/*-- Symbolic Constant Macros (defines) --*/

#define HM_HSP_ERROR_REGISTRATION_OFFSET (0)
#define HM_HSP_ERROR_INJECTION_OFFSET (128)
#define HM_HSP_ERROR_RECORD_OFFSET (192)

#define MBOX_HMM_HEADER_SIZE    (sizeof(large_fifo_mailbox_msg_header) + sizeof(uint32_t))/sizeof(uint32_t) // 32-bit allocated for transfer metadata
#define MBOX_HMM_DATA_DEPTH        (LARGE_FIFO_MBOX_FIFO_DEPTH - MBOX_HMM_HEADER_SIZE)

// this will go away once new silib is available
#ifndef ICC_GEN_CMD
#define ICC_GEN_CMD(module, cmd)         ((module << 16) + cmd)
#endif

// The accelerators used a large fifo mbx which only has 16-bit for commands
// Hence defining a second command which only 16-bit wide
#ifndef ICC_ACCEL_GEN_CMD
#define ICC_ACCEL_GEN_CMD(module, cmd)         (((module << 8) + cmd)&0xFFFF)
#endif

#ifndef ICC_ACCEL_CMDS_WIDTH
#define ICC_ACCEL_CMDS_WIDTH        (0x10)
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

#ifndef ICC_HM_ERROR_INJECTION_AP2SCP
#define ICC_HM_ERROR_INJECTION_AP2SCP          ICC_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, 0x4)
#endif

#ifndef ICC_HM_CPER_TRANSFER_REQ_MCP
#define ICC_HM_CPER_TRANSFER_REQ_MCP          ICC_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, 0x5)
#endif

#ifndef ICC_HM_CPER_TRANSFER_PLDM_REQ_MCP
#define ICC_HM_CPER_TRANSFER_PLDM_REQ_MCP          ICC_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, 0x6)
#endif

#ifndef ICC_HM_ERROR_INJECTION_SETUP_REQ
#define ICC_HM_ERROR_INJECTION_SETUP_REQ          ICC_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, 0x7)
#endif

#ifndef ICC_HM_ERROR_INJECTION_ROUTING_REQ
#define ICC_HM_ERROR_INJECTION_ROUTING_REQ          ICC_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, 0x8)
#endif

#ifndef ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL
#define ICC_HM_ERROR_DOMAIN_REGISTER_ACCEL(accel_id)    ICC_ACCEL_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, (accel_id * ICC_ACCEL_CMDS_WIDTH) + 0x1)
#endif

#ifndef ICC_HM_ERROR_DOMAIN_REGISTER_DONE_ACK_ACCEL
#define ICC_HM_ERROR_DOMAIN_REGISTER_DONE_ACK_ACCEL(accel_id)          ICC_ACCEL_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, (accel_id * ICC_ACCEL_CMDS_WIDTH) + 0x2)
#endif

#ifndef ICC_HM_ERROR_RECORD_SUBMIT_ACCEL
#define ICC_HM_ERROR_RECORD_SUBMIT_ACCEL(accel_id)      ICC_ACCEL_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, (accel_id * ICC_ACCEL_CMDS_WIDTH) + 0x3)
#endif

#ifndef ICC_HM_ERROR_INJECTION_ACCEL
#define ICC_HM_ERROR_INJECTION_ACCEL(accel_id)          ICC_ACCEL_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, (accel_id * ICC_ACCEL_CMDS_WIDTH) + 0x4)
#endif

#ifndef ICC_HM_TX_DONE_ACK_ACCEL
#define ICC_HM_TX_DONE_ACK_ACCEL(accel_id)          ICC_ACCEL_GEN_CMD(ICC_MODULE_HEALTH_MONITOR, (accel_id * ICC_ACCEL_CMDS_WIDTH) + 0x5)
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

typedef union {
    struct {
        large_fifo_mailbox_msg_header header;
        uint16_t error_domain_idx;
        uint8_t valid_fru_id;
        uint8_t valid_fru_text;
        guid_t fru_id;
        char fru_text[ACPI_FRU_TEXT_LENGTH];
    };
    uint32_t as_uint32[LARGE_FIFO_MBOX_FIFO_DEPTH];
} hm_accel_error_domain_register_payload_t;

typedef struct {
    icc_mhu_header_t header;
    hm_error_record_t error_record;
} hm_mhu_error_record_payload_t;

typedef struct {
    icc_mhu_header_t header;
    ras_einj_info_t error_injection_info;
} hm_mhu_error_injection_payload_t;

typedef union {
    struct
    {
        large_fifo_mailbox_msg_header header;
        ras_einj_info_t error_injection_info;
    };
    uint32_t as_uint32[LARGE_FIFO_MBOX_FIFO_DEPTH];
} hm_accel_error_injection_payload_t;

typedef struct _accel_hmm_msg {
	large_fifo_mailbox_msg_header header;
	uint32_t dtcm_mem_offset;
} hm_accel_msg_t;

typedef struct _accel_hmm_msg_ack {
	large_fifo_mailbox_msg_header header;
	uint32_t cper_buffer_offset;
} hm_accel_msg_ack_t;

typedef struct _hm_accel_cper_payload_s
{
    acpi_err_sec_accel_vendor_t accel_err_payload;
    fpfw_icc_base_recv_req_t hm_icc_sdm_err_submit_req;
    hm_accel_msg_t msg_payload;
    uint16_t error_domain_index;
    ACCEL_ID accel_id;
    acpi_error_severity_t err_severity;
} hm_accel_cper_info_t;

typedef struct _hm_accel_fatal_cper_s
{
    uint32_t cper_buffer_offset;
    uint32_t cper_magic_nr_offset;
    bool is_valid;
    bool send_default_cper;
} hm_accel_fatal_cper_t;
