//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ddrss.h
 */

#pragma once

/*----------- Nested includes ------------*/
#include <ddr_erg0_regs.h>
#include <ddr_erg1_regs.h>
#include <ddrmctop_regs.h>
#include <idsw_kng.h>
#include <silibs_platform.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DDRSS_PRINT(lvl, statement)  
#define DDRSS_ERROR(...) DDRSS_PRINT(DDRSS_DEBUG_LEVEL_CRITICAL, CRITICAL_PRINT("ERROR: " __VA_ARGS__))

// ddrMcUpTop
#define DDRSSTOP_AXU_MC0_ADDRESS (0x0U)
#define DDRSSTOP_AXU_MC1_ADDRESS (0x200000U)

#define DDRSS_MC_TOP_ADDR_GAP (DDRSSTOP_AXU_MC1_ADDRESS - DDRSSTOP_AXU_MC0_ADDRESS)

#define DDRSS_MC_RASERG0_OFF(x) ((uint32_t)(uintptr_t) & (((ptr_ddr_erg0_reg)0)->x))
#define DDRSS_MC_RASERG1_OFF(x) ((uint32_t)(uintptr_t) & (((ptr_ddr_erg1_reg)0)->x))

// ddr_erg0
#define DDRMCUPTOP_RASNERG0_ADDRESS (0xe0000U)

// ddr_erg1
#define DDRMCUPTOP_RASNERG1_ADDRESS (0xe8000U)

#define DDRSS_MC_REG_OFF(x) ((uint32_t)(uintptr_t) & (((ptr_ddrmctop_reg)0)->x))  //sksk - this is in ddrmctop_regs_local_copy.h

#define PROD_DDRSS_MC0_RASERG_REG_ADDR(base, reg) (base + DDRMCUPTOP_RASNERG0_ADDRESS + DDRSS_MC_RASERG0_OFF(reg))
#define PROD_DDRSS_MC1_RASERG_REG_ADDR(base, reg) (base + DDRMCUPTOP_RASNERG0_ADDRESS + DDRSS_MC_RASERG0_OFF(reg) + DDRSS_MC_TOP_ADDR_GAP)

// Defines in siLibs private_include folder:
#define csr_PhyAcsmParityErrEn_MASK 0x100u
#define csr_PhyPIEParityErrEn_MASK 0x200u
#define csr_PhyRdfPtrChkErrEn_MASK 0x400u
#define csr_PhyEccEn_MASK 0x800u
#define csr_PhyPIEProgErrEn_MASK 0x1000u
#define csr_PhyTxPPTEn_MASK 0x2000u
#define csr_PhyAlertEn_MASK 0x4000u

#define tDRTUB 0xc0000u
#define csr_ArcEccIndications_ADDR 0x82u
/*-------------- Typedefs ----------------*/
typedef struct
{
    KNG_DIE_ID die_id;
    uint32_t ddrss_num;
} ddrss_isr_params_t;
/* Internal parameters calculated from configurations */

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void prod_ddrss_lib_init(KNG_DIE_ID die_num);
int prod_ddrss_phy_interrupt_handler(uint32_t mc);
int prod_ddrss_mc_interrupt_handler(uint32_t mc);
void prod_ddrss_interrupt_handler(void *context);
uintptr_t ddrss_get_top_base(uint32_t mc);

// in ddrss_ras.c
int ddrss_probe_ras_agent(uint32_t mc, uint32_t ras_agent_entity_id);
