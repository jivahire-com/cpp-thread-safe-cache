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
#include <atu_lib.h>
#include <cper.h>
#include <idsw_kng.h>             // for KNG_DIE_ID, idsw_get_die_id
#include <mscp_error_domain.h>
#include <nvic.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MMIO_SET_MASK32(addr, mask)   MMIO_WRITE32(addr, (MMIO_READ32(addr) | (mask)))
#define MMIO_CLEAR_MASK32(addr, mask) MMIO_WRITE32(addr, (MMIO_READ32(addr) & ~(mask)))

#define ARSM_RAM_DEFAULT_OFFSET (0x10)
#define TCM_TGT_RAM_DTCM0RAM (0x80)
#define MASK_CE              0x20
#define MASK_UE              0x40

/*------------- Typedefs -----------------*/
typedef void (*ecc_entry_getter_fn)(int type, atu_map_entry_t* entry);

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern const atu_map_entry_t s_hm_arsm_atu_entries[2][MSCP_ARSM_RAM_COUNT];

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
void register_scp_ecc_isr_with_param(uint32_t irq_num, isr_callback_fn_with_params_t isr, void* isr_param);
void inject_err_by_access(uint32_t addr);
void trigger_shared_sram_fault(bool arsm, int type, uint32_t target_addr, uint32_t err_mask);
void trigger_shared_sram_arsm_fault(mscp_arsm_ram_type_t type, uint32_t target_addr, uint32_t err_mask);
void shared_sram_ecc_isr(void* ctx);
void trigger_shared_sram_rsm_fault(mscp_rsm_ram_type_t type, uint32_t target_addr, uint32_t err_mask);
void unmap_rsm_address(atu_map_entry_t* atu_entry);
void shared_sram_ecc_isr_ext();
void enable_shared_sram_errors(ecc_entry_getter_fn get_entry, int count);
void get_rsm_ecc_atu_entry_wrapper(int type, atu_map_entry_t* entry);
acpi_einj_cmd_status_t hm_smmu_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx);
uint32_t map_rsm_address(atu_map_entry_t* atu_entry);
acpi_einj_cmd_status_t gic_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx);
acpi_einj_cmd_status_t mscp_error_injection_handler(ras_einj_info_t* einj_payload, void* ctx);
