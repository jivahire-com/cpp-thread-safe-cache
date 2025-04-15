//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file snsr_fifo_debug_i.h
 * Debug mode where raw data is collected from the sensor fifo and stored in DDR
 */

 #pragma once

 /*----------- Nested includes ------------*/

 /*-- Symbolic Constant Macros (defines) --*/

 /*-------------- Typedefs ----------------*/

 /*-- Declarations (Statics and globals) --*/

extern uintptr_t sfd_pkg_start_location;
extern size_t sfd_pkg_available_size;
extern uint32_t sfd_pkg_used_size;
extern uint32_t sfd_next_record_number;
extern uint32_t sfd_package_number;

 /*--------- Function Prototypes ----------*/

void add_pstate_collection(void);
void add_tile_temperature_collection(void);
void add_tile_voltage_collection(void);
void add_core_current_collection(void);
void add_soc_pvt_temperature_collection(void);
void add_soc_pvt_voltage_collection(void);
void add_dimm_info_collection(void);
void add_vr_temperature_collection(void);
void add_vr_current_collection(void);