//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sensor_fifo_events_i.h
 * Event traces for the sensor fifo service
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h> // for EVENT_TRACE_PROVIDER_ID_SCP_FUSE
#include <event_trace.h>
#include <stdint.h>                // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_COMMON_SENSOR_FIFO_SVC,
                           SnsrFifoSvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_SENSOR_FIFO_SVC,
                     10,
                     GlobalEnableFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_SENSOR_FIFO_SVC,
                     11,
                     FifoEnableFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))


FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_SENSOR_FIFO_SVC,
                     12,
                     FifoDisableFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32, status))


/*--------- Function Prototypes ----------*/