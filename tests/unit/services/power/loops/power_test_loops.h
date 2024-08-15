/*
 * Copyright (c) Microsoft Corporation.
 */

/**
 * @file power_test_loops.h
 *
 */

#pragma once

/*----------- Includes ------------*/
#include <cstddef>

extern "C" {
#include <power_loops_i.h>
}

/*-- Symbolic Constant Macros (defines) --*/


/*-------------- Typedefs ----------------*/
// name the indexes for control/telemetry types
enum : unsigned int {
    CTRL = 0,
    VR_TELEM = 0,
    PVT_TELEM = 1,
    LOOP_MAX,
};
/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
void call_handler(unsigned int index, int state, int signal, const void* data);
void reset_context();
power_loop_context_t* loop_context(unsigned int index);
void setup_expectations_for_state_change(int state);
void setup_expectations_for_retry_fail(power_loop_retries_t type, bool fail);
