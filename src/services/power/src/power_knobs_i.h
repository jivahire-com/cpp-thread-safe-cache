//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file power_knobs_i.h
 * Private header for power configuration knobs
 */

#pragma once

/*----------- Nested includes ------------*/
/*-- Symbolic Constant Macros (defines) --*/
// emulation platforms have PVT models (for analog part) that produce values out of expected ranges, these
// values are within those ranges, primarily to avoid thermtrip
#define POWER_KNOB_THERMTRIP_ALARM_PVT_MODEL 2000
#define POWER_KNOB_HOT_ALARM_PVT_MODEL       1700
// define a maximum percentage
#define POWER_KNOB_PERCENTAGE_MAX 100
// ranges for HW current throttling
#define POWER_KNOB_CURRTHROT_TEL_EPOCH_COUNT_MAX      20
#define POWER_KNOB_CURRTHROT_TEL_EPOCH_COUNT_MIN      10
#define POWER_KNOB_CURRTHROT_ROLLING_WINDOW_COUNT_MAX 7
// general HW throttling cnt/step ranges
#define POWER_KNOB_THROT_CNT_1F 0x1F
#define POWER_KNOB_THROT_CNT_F  0xF
#define POWER_KNOB_THROT_STEP_8 7
#define POWER_KNOB_THROT_STEP_4 3
// power SW maximums
#define POWER_KNOB_STEP_SIZE_MAX        31
#define POWER_KNOB_FORCE_PSTATE_DISABLE 32
#define POWER_KNOB_PLIMIT_MAX           31
// VR CPU max voltage (mV) in survivability mode
#define POWER_KNOB_VR_FORCED_VCPU_SURVIVABILITY 963
// power control loop interval for emulation platforms
#define POWER_KNOB_CTRL_LOOP_INTERVAL_PVT_MODEL 50
/*-------------- Typedefs ----------------*/
/*-- Declarations (Statics and globals) --*/
/*--------- Function Prototypes ----------*/
