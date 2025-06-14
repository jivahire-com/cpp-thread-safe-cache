//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_pex.h
 * Header file of error domain PEX
 */
#pragma once

/*----------- Nested includes ------------*/
#include <idsw_kng.h> // for KNG_DIE_ID, idsw_get_die_id
#include <pex_regs.h>
#include <pex_rng.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

#if defined (SCP_RUNTIME_INIT)

void cons_pex_isr(void* context);
void pex_irq_handle(KNG_DIE_ID die_num, uint32_t pex_num, pex_rng_config_t* rng_cfg);

#endif