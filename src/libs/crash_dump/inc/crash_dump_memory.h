//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_memory.h
 * Specifies memory regions used for crash dump
 */
#pragma once
#include <assert.h>
#include <atu_api.h>
#include <crash_dump.h>
#include <mscp_exp_rmss_memory_map.h>

/*----------- Nested includes ------------*/

/*-- Symbolic Constant Macros (defines) --*/
// Alignment macros
#ifndef DUMP_ALIGNMENT
#define DUMP_ALIGNMENT (8)
#endif

#define CD_ALIGN_BY(VALUE, INTERVAL) (((VALUE) + (INTERVAL)-1) & ~((INTERVAL)-1))
#define CD_ALIGN_DOWN(VALUE, INTERVAL) ((VALUE) / (INTERVAL) * (INTERVAL))

// Mini dump is not supported for MCP and Accelerators because they are booted up after DDR is up and running.
// DIE0                         DIE1
// +-------------------------+  +-------------------------+ CRASH_DUMP_MINI_HEADER_ADDR (CRASH_DUMP_MINI_HEADER_SIZE)
// |  Crash Dump Header      |  |  Crash Dump Header      |
// +-------------------------+  +-------------------------+ CRASH_DUMP_MINI_SCP_ADDR (CRASH_DUMP_MINI_SCP_SIZE)
// |  DIE0 SCP Crash Dump    |  |  DIE1 SCP Crash Dump    |
// +-------------------------+  +-------------------------+ CRASH_DUMP_MINI_MCP_ADDR (CRASH_DUMP_MINI_MCP_SIZE)
// |  DIE0 MCP Crash Dump    |  |  DIE1 MCP Crash Dump    |
// +-------------------------+  +-------------------------+
#define CRASH_DUMP_MINI_SIZE_PER_CORE   CD_ALIGN_DOWN(SCP_EXP_CRASHDUMP_MINI_MAX_SIZE / 2, DUMP_ALIGNMENT)

#define CRASH_DUMP_MINI_HEADER_ADDR SCP_EXP_CRASHDUMP_HEADER_BASE
#define CRASH_DUMP_MINI_HEADER_SIZE CD_ALIGN_BY(SCP_EXP_CRASHDUMP_HEADER_SIZE, DUMP_ALIGNMENT)
static_assert(sizeof(crash_dump_header_t) <= CRASH_DUMP_MINI_HEADER_SIZE, "crash_dump_header_t size is larger than CRASH_DUMP_MINI_HEADER_SIZE");

#define CRASH_DUMP_MINI_SCP_ADDR    CD_ALIGN_BY(SCP_EXP_CRASHDUMP_MINI_BASE, DUMP_ALIGNMENT)
#define CRASH_DUMP_MINI_SCP_SIZE    CD_ALIGN_DOWN(CRASH_DUMP_MINI_SIZE_PER_CORE, DUMP_ALIGNMENT)

#define CRASH_DUMP_MINI_MCP_ADDR    CD_ALIGN_BY(CRASH_DUMP_MINI_SCP_ADDR + CRASH_DUMP_MINI_SCP_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_MINI_MCP_SIZE    CD_ALIGN_DOWN(CRASH_DUMP_MINI_SIZE_PER_CORE, DUMP_ALIGNMENT)
static_assert(CRASH_DUMP_MINI_MCP_ADDR + CRASH_DUMP_MINI_MCP_SIZE - 1 <= SCP_EXP_CRASHDUMP_MINI_END, "Mini crash dump memory region exceeds SCP_EXP_CRASHDUMP_MINI_END");

// +-------------------------+   CRASH_DUMP_FULL_STATUS_ADDR (CRASH_DUMP_STATUS_SIZE)
// |  Crash Dump Header      |
// +-------------------------+   CRASH_DUMP_FULL_MCP0_ADDR (CRASH_DUMP_FULL_MCP_SIZE)
// |  DIE0 MCP Crash Dump    | 
// +-------------------------+   CRASH_DUMP_FULL_SCP0_ADDR (CRASH_DUMP_FULL_SCP_SIZE)
// |  DIE0 SCP Crash Dump    | 
// +-------------------------+   CRASH_DUMP_FULL_HSP0_ADDR (CRASH_DUMP_FULL_HSP_SIZE)
// |  DIE0 HSP Crash Dump    | 
// +-------------------------+   CRASH_DUMP_FULL_CDED0_ADDR (CRASH_DUMP_FULL_CDED_SIZE)
// |  DIE0 CDED Crash Dump   | 
// +-------------------------+   CRASH_DUMP_FULL_SDM0_ADDR (CRASH_DUMP_FULL_SDM_SIZE)
// |  DIE0 SDM Crash Dump    | 
// +-------------------------+   CRASH_DUMP_FULL_KMP0_ADDR (CRASH_DUMP_FULL_KMP_SIZE)
// |  DIE0 KMP Crash Dump    | 
// +-------------------------+   CRASH_DUMP_FULL_MCP1_ADDR (CRASH_DUMP_FULL_MCP_SIZE)
// |  DIE1 MCP Crash Dump    |
// +-------------------------+   CRASH_DUMP_FULL_SCP1_ADDR (CRASH_DUMP_FULL_SCP_SIZE)
// |  DIE1 SCP Crash Dump    |
// +-------------------------+   CRASH_DUMP_FULL_HSP1_ADDR (CRASH_DUMP_FULL_HSP_SIZE)
// |  DIE1 HSP Crash Dump    |
// +-------------------------+   CRASH_DUMP_FULL_CDED1_ADDR (CRASH_DUMP_FULL_CDED_SIZE)
// |  DIE1 CDED Crash Dump   |
// +-------------------------+   CRASH_DUMP_FULL_SDM1_ADDR (CRASH_DUMP_FULL_SDM_SIZE)
// |  DIE1 SDM Crash Dump    |
// +-------------------------+   CRASH_DUMP_FULL_KMP1_ADDR (CRASH_DUMP_FULL_KMP_SIZE)
// |  DIE1 KMP Crash Dump    |
// +-------------------------+ 

// Full crash dump address is ATU translated address
#define CRASH_DUMP_FULL_HEADER_ADDR MSCP_ATU_AP_WINDOW_CRASH_DUMP_BASE_ADDR
#define CRASH_DUMP_FULL_HEADER_SIZE (8 * SL_1KB)
static_assert(sizeof(crash_dump_header_t) <= CRASH_DUMP_FULL_HEADER_SIZE, "crash_dump_header_t size is larger than CRASH_DUMP_FULL_HEADER_SIZE");

#define CRASH_DUMP_FULL_SIZE_PER_DIE    ((CRASH_DUMP_RESERVATION_END - CRASH_DUMP_RESERVATION_BASE - CRASH_DUMP_FULL_HEADER_SIZE) / 2) // 4MB for each die
#define CRASH_DUMP_FULL_SIZE_PER_CORE   CD_ALIGN_DOWN(CRASH_DUMP_FULL_SIZE_PER_DIE / CRASH_DUMP_CORE_NUM, DUMP_ALIGNMENT)

// Full crash dump payload starts right after the header
#define CRASH_DUMP_FULL_PAYLOAD_ADDR CD_ALIGN_BY(CRASH_DUMP_FULL_HEADER_ADDR + CRASH_DUMP_FULL_HEADER_SIZE, DUMP_ALIGNMENT)

// Die 0 crash dump regions
#define CRASH_DUMP_FULL_MCP0_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_PAYLOAD_ADDR, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_MCP0_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_SCP0_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_MCP0_ADDR + CRASH_DUMP_FULL_MCP0_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_SCP0_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_HSP0_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_SCP0_ADDR + CRASH_DUMP_FULL_SCP0_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_HSP0_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_CDED0_ADDR   CD_ALIGN_BY(CRASH_DUMP_FULL_HSP0_ADDR + CRASH_DUMP_FULL_HSP0_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_CDED0_SIZE   CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_SDM0_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_CDED0_ADDR + CRASH_DUMP_FULL_CDED0_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_SDM0_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_KMP0_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_SDM0_ADDR + CRASH_DUMP_FULL_SDM0_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_KMP0_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

// Die 1 crash dump regions
#define CRASH_DUMP_FULL_MCP1_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_KMP0_ADDR + CRASH_DUMP_FULL_KMP0_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_MCP1_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_SCP1_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_MCP1_ADDR + CRASH_DUMP_FULL_MCP1_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_SCP1_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_HSP1_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_SCP1_ADDR + CRASH_DUMP_FULL_SCP1_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_HSP1_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_CDED1_ADDR   CD_ALIGN_BY(CRASH_DUMP_FULL_HSP1_ADDR + CRASH_DUMP_FULL_HSP1_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_CDED1_SIZE   CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_SDM1_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_CDED1_ADDR + CRASH_DUMP_FULL_CDED1_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_SDM1_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

#define CRASH_DUMP_FULL_KMP1_ADDR    CD_ALIGN_BY(CRASH_DUMP_FULL_SDM1_ADDR + CRASH_DUMP_FULL_SDM1_SIZE, DUMP_ALIGNMENT)
#define CRASH_DUMP_FULL_KMP1_SIZE    CRASH_DUMP_FULL_SIZE_PER_CORE

static_assert(CRASH_DUMP_FULL_KMP1_ADDR + CRASH_DUMP_FULL_KMP1_SIZE - 1 <= MSCP_ATU_AP_WINDOW_CRASH_DUMP_END_ADDR, "Full crash dump memory region exceeds MSCP_ATU_AP_WINDOW_CRASH_DUMP_END_ADDR");

#define CRASH_DUMP_CORE_ADDRESS(die_id, core_id) \
    ((uint8_t*)(intptr_t)(CRASH_DUMP_FULL_PAYLOAD_ADDR + ((die_id) * CRASH_DUMP_FULL_SIZE_PER_DIE) + ((core_id) * CRASH_DUMP_FULL_SIZE_PER_CORE)))

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
