//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file exec_tlm_cmpnt_i.h
 * Private include file for the telemetry executive component for testing access.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

// event flags
#define PWR_AGGR_TMR_EXPIRED       (1 << 0)
#define INST_SAMPLE_TMR_EXPIRED    (1 << 1)
#define PWR_PKG_TMR_EXPIRED        (1 << 2)
#define EVERY_24HR_PKG_TMR_EXPIRED (1 << 3)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern TX_TIMER pwr_aggr_tmr;
extern TX_TIMER inst_sample_tmr;
extern TX_TIMER power_pkg_tmr;
extern TX_TIMER _24hr_pkg_tmr;

extern telemetry_executive_status_t tlm_executive_status;

/*--------- Function Prototypes ----------*/
void tlm_svc_thread(ULONG thread_input);
void pwr_aggr_timer_cb(ULONG context);
void inst_sample_timer_cb(ULONG context);
void pwr_pkg_timer_cb(ULONG context);
void every_24hr_pkg_timer_cb(ULONG context);