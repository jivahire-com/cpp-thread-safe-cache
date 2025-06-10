//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_i.h
 *
 * Private header file for supporting SCP ras error
 */
#pragma once

/*------------- Includes -----------------*/
#include <cper.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MMIO_SET_MASK32(addr, mask)   MMIO_WRITE32(addr, (MMIO_READ32(addr) | (mask)))
#define MMIO_CLEAR_MASK32(addr, mask) MMIO_WRITE32(addr, (MMIO_READ32(addr) & ~(mask)))

#define TCM_TGT_RAM_DTCM0RAM (0x80)
#define MASK_CE              0x20
#define MASK_UE              0x40

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
void dcache_ue_isr();
void dcache_ce_isr();
void dcache_tag_ue_isr();
void dcache_tag_ce_isr();
void icache_ue_isr();
void icache_ce_isr();
void icache_tag_ue_isr();
void icache_tag_ce_isr();
void trigger_hard_fault();
void trigger_bus_fault();
void trigger_usage_fault();
void trigger_mmu_fault();
void trigger_mscp_watchdog_fault();
acpi_einj_cmd_status_t mscp_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx);