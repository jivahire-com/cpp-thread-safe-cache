//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file d2d_cntr_sync.h
 * Public header file of d2d_cntr_sync
 */
#pragma once

/*----------- Nested includes ------------*/
#include <idsw_kng.h> // for KNG_DIE_ID
#include <stdint.h> // for int32_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 * @brief Configures the d2d_cntr_sync module for each die 
 * to synchronize the counter values and enables periodic sync
 * Only for SCP core. Not supported on SVP currently.
 * 
 * @param  die_num - die number, each die has separate cfg params
 */
void d2d_cntr_sync_init(KNG_DIE_ID die_num, KNG_CPU_TYPE cpu_type);

/**
 * @brief : To be called in mesh init post d2dss_sequence()
 * Only for SCP core die 1. Not supported on SVP currently.
 */
void d2d_cntr_sync_enable(void);

/**
 * @brief Destructor for d2d_cntr_sync module. Resets the state of the module.
 * use ONLY for unit tests.
 */
void d2d_cntr_sync_reset(void);