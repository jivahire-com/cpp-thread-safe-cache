//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_status.h
 * Crash dump status management functions
 */
#pragma once

/*----------- Nested includes ------------*/
#include <crash_dump.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
/**
 * @brief Initialize crash dump header or wait until it's initialized.
 *
 * @param type_context Crash dump type context.
 */
void initialize_crash_dump_header(crash_dump_type_context_t *type_context);

/**
 * @brief Update crash dump region state.
 * 
 * @param type_context Crash dump type context.
 * @param state 1 in used, 0 not in use
 * 
 */
void crash_dump_update_state(crash_dump_type_context_t *type_context, crash_dump_state_t state);

/**
 * @brief Update crash dump state of core.
 *
 * @param type_context Crash dump type context.
 * @param state Desired state of the core.
 * 
 */
void crash_dump_update_core_state(crash_dump_type_context_t *type_context, crash_dump_core_state_t state);

/**
 * @brief Dump crash dump status.
 * 
 * @param type_context Crash dump type context. If null dump all types.
 */
void crash_dump_dump_status(crash_dump_type_context_t *type_context);

/**
 * @brief Update crash dump state of accelerators.
 * 
 * @param accel_type Accelerator type.
 * @param state Desired state of the accelerator.
 */
void crash_dump_update_accel_state(ACCEL_ID accel_type, crash_dump_core_state_t state);

/*-------- Function Prototypes -----------*/
/**
 * Returns DDR crash dump size
 *
 * @return uint32_t
 *      Crash dump size
 */
uint32_t crash_dump_get_dump_size(uint8_t die_id, crash_dump_core_t core_id);
/**
 * Returns whether dump is complete
 *
 * @return bool
 *      Is dump complete
 */
bool crash_dump_is_dump_complete(uint8_t die_id, crash_dump_core_t core_id);