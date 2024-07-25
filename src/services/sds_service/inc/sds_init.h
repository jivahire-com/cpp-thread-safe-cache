//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sds_init.h
 * Header containing definitions for initialization of sds service
 */

#pragma once

/*----------- Nested includes ------------*/
#include "sds_api.h"
#include "sds_configuration.h"
#include <FpFwUtils.h>

/*-- Symbolic Constant Macros (defines) --*/
#define REGION_SIGNATURE 0xAA7A
/* The minor version of the SDS schema supported by this implementation */
#define SUPPORTED_VERSION_MINOR 0x0
/* The major version of the SDS schema supported by this implementation */
#define SUPPORTED_VERSION_MAJOR 0x1
#define MODULE_NAME "[SDS_SVC] "
#define NEWLINE     "\n"

#define SDS_ALIGNMENT_SIZE 8
#define SDS_ALIGN(size) (FPFW_ALIGN_BY(SDS_ALIGNMENT_SIZE, size))
#define SDS_MIN_REGION_SIZE (FPFW_ALIGN_BY(SDS_ALIGNMENT_SIZE, (sizeof(sds_region_header_t) + sizeof(sds_block_header_t))))

#define SDS_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define SDS_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define SDS_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

/*-------------- Typedefs ----------------*/
typedef struct {
    uint16_t signature;
    uint16_t block_count : 8;
    uint16_t version_minor : 4;
    uint16_t version_major : 4;
    uint32_t region_size;
} sds_region_header_t;

typedef struct {
    struct {
        uint32_t free_mem_size;
        volatile char *free_mem_base;
    } sds_regions[PLATFORM_SDS_REGION_COUNT];
} sds_current_context_t;

typedef struct structure_header {
    uint32_t sds_block_id;
    bool valid : 1;
    uint32_t sds_block_size : 23;
    uint32_t reserved : 8;
} sds_block_header_t;

/* Module context */

/*--------- Function Prototypes ----------*/
void sds_init(psds_service_t p_device, PDFWK_SCHEDULE p_schedule);
void sds_interface_init(psds_service_t p_device, psds_service_interface_t p_interface);