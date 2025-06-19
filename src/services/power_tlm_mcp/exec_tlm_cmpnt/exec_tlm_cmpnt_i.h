//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file exec_tlm_cmpnt_i.h
 * Private include file for the telemetry executive component for testing access.
 */

#pragma once

/*----------- Nested includes ------------*/

#include "exec_tlm_cmpnt.h"
#include <stdint.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

// event flags
#define DATA_AGGR_TMR_EXPIRED      (1 << 0)
#define MODE_CHANGE_PENDING        (1 << 1)
#define INST_SAMPLE_TMR_EXPIRED    (1 << 2)
#define PWR_PKG_TMR_EXPIRED        (1 << 3)
#define EVERY_24HR_PKG_TMR_EXPIRED (1 << 4)
#define NEW_INBAND_MTS_MESSAGE     (1 << 5)
#define NEW_OUT_OF_BAND_PLDM_REQ   (1 << 6)

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/
extern TX_TIMER data_aggr_tmr;
extern TX_TIMER inst_sample_tmr;
extern TX_TIMER power_pkg_tmr;
extern TX_TIMER _24hr_pkg_tmr;

extern tlm_operating_mode_t pending_mode_change;

extern telemetry_executive_status_t tlm_executive_status;

/*--------- Function Prototypes ----------*/
void tlm_svc_thread(ULONG thread_input);
void data_aggr_timer_cb(ULONG context);
void inst_sample_timer_cb(ULONG context);
void pwr_pkg_timer_cb(ULONG context);
void every_24hr_pkg_timer_cb(ULONG context);

void run_timer_exit_actions(tlm_operating_mode_t exiting_mode);
void run_timer_enter_actions(tlm_operating_mode_t entering_mode);