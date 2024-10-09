//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file in_band_telemetry_ddr.h
 *  Header for any in-band telemetry defines, macros, etc.. to reflect how the
 *  DDR is split up for in-band telemetry data.
*/

#pragma once

/*----------- Nested includes ------------*/

#include <assert.h>
#include <atu_api.h>
#include <stdint.h>
#include <utils.h>

/*-- Symbolic Constant Macros (defines) --*/

/**
 * All in-band telemetry data is transferred into DDR (to support reading by the host).
 * All clients share the same range of DDR, and therefore it needs to split it up by
 * client and die.  atu service is responsible for mapping the DDR into the AP window. Atu handles
 * the die split of the total inband telemetry DDR range. Therefore, MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR is the
 * same for both both dies, but the window is mapped to die specific DDR ranges.
*/


/**
 * Event Trace uses 16 MB of DDR per die, and starts at the base of each
 * die's DDR range. It's used for both ASIC buffers and HSP buffers.
*/
#define IB_TLM_DDR_ATU_AP_WIN_BASE_ADDR (MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_BASE_ADDR)
#define IB_TLM_DDR_ATU_AP_WIN_END_ADDR (MSCP_ATU_AP_WINDOW_IB_TELEMETRY_DIE_END_ADDR)

#define IB_TLM_DDR_ATU_AP_WIN_TRACE_SIZE (MB * 16)
#define IB_TLM_DDR_ATU_AP_WIN_TRACE_BASE_ADDR (IB_TLM_DDR_ATU_AP_WIN_BASE_ADDR)
#define IB_TLM_DDR_ATU_AP_WIN_TRACE_END_ADDR (IB_TLM_DDR_ATU_AP_WIN_TRACE_BASE_ADDR + IB_TLM_DDR_ATU_AP_WIN_TRACE_SIZE)

#define IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE (1 * MB)
#define IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_BASE_ADDR (IB_TLM_DDR_ATU_AP_WIN_TRACE_BASE_ADDR)
#define IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_END_ADDR (IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_BASE_ADDR + IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE)

#define IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE (15 * MB)
#define IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_BASE_ADDR (IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_END_ADDR)
#define IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_END_ADDR (IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_BASE_ADDR + IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE)

static_assert(IB_TLM_DDR_ATU_AP_WIN_TRACE_SIZE == (IB_TLM_DDR_ATU_AP_WIN_TRACE_HSP_SIZE + IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_SIZE), "Trace DDR DIE sizes do not add up to total DDR DIE Trace size");

#define IB_TLM_DDR_ATU_AP_WIN_SCHEMA_SIZE (MB * 1)
#define IB_TLM_DDR_ATU_AP_WIN_SCHEMA_BASE_ADDR (IB_TLM_DDR_ATU_AP_WIN_TRACE_ASIC_END_ADDR)
#define IB_TLM_DDR_ATU_AP_WIN_SCHEMA_END_ADDR (IB_TLM_DDR_ATU_AP_WIN_SCHEMA_BASE_ADDR + IB_TLM_DDR_ATU_AP_WIN_SCHEMA_SIZE)

// use remaining DDR space for power telemetry
#define IB_TLM_DDR_ATU_AP_WIN_PWR_TLM_SIZE (IB_TLM_DDR_ATU_AP_WIN_END_ADDR - IB_TLM_DDR_ATU_AP_WIN_SCHEMA_END_ADDR)
#define IB_TLM_DDR_ATU_AP_WIN_PWR_TLM_BASE_ADDR (IB_TLM_DDR_ATU_AP_WIN_SCHEMA_END_ADDR)
#define IB_TLM_DDR_ATU_AP_WIN_PWR_TLM_END_ADDR (IB_TLM_DDR_ATU_AP_WIN_END_ADDR)

/*-------------- Typedefs ----------------*/


/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
