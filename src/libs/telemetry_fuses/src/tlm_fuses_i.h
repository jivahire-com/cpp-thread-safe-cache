//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file tlm_fuses_i.h
 * Private header for the telemetry fuse library.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <fpfw_status.h>
#include <stdint.h>
#include <tlm_fuses.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

fpfw_status_t tlm_fuses_read(const uintptr_t fuse_store_addr, const unsigned int fuse_bit_offset, const unsigned int fuse_bit_size);
fpfw_status_t tlm_fuses_get_dts_coeff(unsigned int k_offset,
                                             uint32_t k_width,
                                             unsigned int y_offset,
                                             uint32_t y_width,
                                             uint32_t coeff_count,
                                             uint32_t coeff_spacing,
                                             dts_tlm_coeff_t* dts_coeff);