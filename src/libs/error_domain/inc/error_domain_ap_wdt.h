//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_ap_wdt.h
 * Header file of error domain AP_WDT
 */
#pragma once

/*----------- Nested includes ------------*/
#include <idsw_kng.h> 


/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

#if defined (SCP_RUNTIME_INIT)

void hm_ap_wdt_isr();
void register_ap_wdt_error_domain(void);

#endif