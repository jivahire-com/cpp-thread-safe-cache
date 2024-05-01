//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_i.h
 * DDR Manager private API
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DDR_WORK_QUEUE_NAME ("ddr-work-queue")
#define DDR_WORK_THREAD_NAME ("ddr-work_thread")
#define DDR_TIMER_NAME ("ddr-timer")

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
void ddr_worker_thread_func(ULONG pddr_service_ctx);
void ddr_timer_cb(ULONG pddr_service_ctx);
void ddr_poll_dimms(void);
void ddr_poll_low_dimms(void);
void ddr_poll_high_dimms(void);
void ddr_create_memory_map(void);
void ddr_process_i3c_data(void);
void ddr_create_bdat(void);
void ddr_create_smbios_tables(void);
void ddr_create_smbios_type_16(void);
void ddr_create_smbios_type_17(void);
void copy_type_16_to_ddr(void);
void copy_type_17_to_ddr(void);