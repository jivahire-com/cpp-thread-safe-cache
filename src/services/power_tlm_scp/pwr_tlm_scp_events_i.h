//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file telemetry_events_i.h
 * Event traces for the telemetry service
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h>
#include <event_trace.h>
#include <stdint.h>                // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/
/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_SCP_TLM_SERVICE,
                           ScpPowerTelemetrySvc,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_TLM_SERVICE,
                     1,
                     ScpMtsMgrClientFreeFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, block_addr),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_TLM_SERVICE,
                     2,
                     ScpMtsMgrClientQueueFail,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, queue),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, tx_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_TLM_SERVICE,
                    3,
                    ScpTlmSvcUnexpectedIncomingDcpMsg,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, client_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, msg_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, seq_num))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_TLM_SERVICE,
                    4,
                    ScpTlmSvcDebugIncomingTrpMsg,
                    FPFW_ET_LEVEL_DEBUG,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, source_seq_num),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, source_core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_die_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, dest_core_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, mts_client_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, trp_msg_id),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_INT16, trp_msg_status),
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, init_cmd))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_TLM_SERVICE,
                    5,
                    ScpMtsMgrClientUnexpectedTrpMsg,
                    FPFW_ET_LEVEL_WARNING,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, trp_msg_id))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_TLM_SERVICE,
                    6,
                    ScpMtsMgrClientUnexpectedCmd,
                    FPFW_ET_LEVEL_WARNING,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, cmd))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_TLM_SERVICE,
                    7,
                    ScpMtsMgrEnablesUpdated,
                    FPFW_ET_LEVEL_INFO,
                    FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT16, enables))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_SCP_TLM_SERVICE,
                    8,
                    ScpCoreVminInitialized,
                    FPFW_ET_LEVEL_INFO)

/*--------- Function Prototypes ----------*/

