//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump_expect.h
 * File containing function prototype to configure test expectations
 */
#pragma once

/*----------- Nested includes ------------*/
#include <modules/CdDumpManager.h>
#include <crash_dump.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
extern "C" {

void set_expectations_gpio_set_output(crash_dump_context_t *context);
void set_expectations_crash_dump_enable_full_dump(crash_dump_context_t* context, crash_dump_header_t* header, bool fulldump);
void set_expectations_init_mem_pool(uint64_t addr, uint32_t size);
void set_expectations_init_dump_desc(bool is_full_dump);
void set_expectations_init_dump_file();
void set_expectations_init_dump_manager(uint32_t expectDumpSize);
void set_expectations_crash_dump_register_core_registers();
void set_expectations_crash_dump_register_default_registers(const core_register_mmio_t* mmio_registers, uint32_t mmio_register_count);
void set_expectations_crash_dump_register_core_stack();
void set_expectations_crash_dump_register_standard_info();
void set_expectations_crash_dump_register_ecid();
void set_expectations_crash_dump_register_threadx(crash_dump_type_context_t *type_context);
void set_expectations_crash_dump_register_address32(void* address_exp, uint32_t size_exp, FPFwCdDumpPriority priority_exp);
void set_expectations_crash_dump_register_address32_no_address(uint32_t size_exp, FPFwCdDumpPriority priority_exp);

}