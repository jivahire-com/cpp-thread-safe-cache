//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file health_monitor_temporary_ras.h
 * Temporary header file for supporting health monitoring until TFA shared header is available
 */
#pragma once

/*------------- Includes -----------------*/
#include <cper.h>
#pragma pack(1)

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/
typedef struct _acpi_ghes_error_record_multi_t
{
    uint32_t block_status_ue : 1;
    uint32_t block_status_ce : 1;
    uint32_t block_status_multi_ue : 1;
    uint32_t block_status_multi_ce : 1;
    uint32_t block_status_entry_count : 10;
    uint32_t block_status_reserved1 : 18;
    uint32_t raw_data_offset;
    uint32_t raw_data_length;
    uint32_t data_length;
    uint32_t error_severity;
    acpi_ghes_error_entry_t data[2];
} acpi_ghes_error_record_multi_t;
#pragma pack()
/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
