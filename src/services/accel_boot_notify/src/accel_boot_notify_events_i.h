//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file accel_boot_notify_events_i.h
 * Event traces for the accelerator boot complete notification service
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h>
#include <event_trace.h>

/*-- Symbolic Constant Macros (defines) --*/

#define ACCEL_BOOT_NOTIFY_ET_INFO_PARAM(stage, type, boot_type)    EventWriteSosInfoParam((stage), (type), (boot_type))

// clang-format off

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_COMMON_ACCEL_BOOT_NOTIFY,
                           AccelBootNotifySvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

//
// Info Events
//

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_ACCEL_BOOT_NOTIFY,
                     0,
                     AccelBootNotifyParam,
                     FPFW_ET_LEVEL_INFO,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, status))

/*--------- Function Prototypes ----------*/
