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
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
extern "C" {

void crash_dump_wait_forever();
void set_expectations_gpio_set_output();
void set_expectations_init_mem_pool();
void set_expectations_init_dump_desc();
void set_expectations_init_dump_file();
void set_expectations_init_dump_manager();
void set_expectations_crash_dump_register_core_registers();
void set_expectations_crash_dump_register_default_registers();
void set_expectations_crash_dump_register_core_stack();
void set_expectations_crash_dump_register_standard_info();
void set_expectations_crash_dump_register_threadx();
void set_expectations_crash_dump_register_address32(void* address_exp, uint32_t size_exp, FPFwCdDumpPriority priority_exp);
void set_expectations_crash_dump_register_address32_no_address(uint32_t size_exp, FPFwCdDumpPriority priority_exp);

}