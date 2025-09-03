//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file mhu_icc_transport_events_i.h
 * Event traces for the icc mhu transport driver
 */

#pragma once

/*----------- Nested includes ------------*/

#include <event_trace_providers.h>
#include <event_trace.h>
#include <stdint.h>                // for UINT8_MAX

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

FPFW_ET_DEFINE_PROVIDER_EX(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                           MhuIccTransport,
                           FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_WARNING | FPFW_ET_LEVEL_MASK_ERROR |
                               FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ALWAYS);

//
// VERBOSE Events - No Offset to Event Id
//
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     0,
                     AsyncSendComplete,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     1,
                     AsyncSendTimerReset,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     2,
                     AsyncSendTimerEnable,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     3,
                     SyncDispatchRequest,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request_type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     4,
                     SyncMaxMessageSizeRequest,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, max_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     5,
                     SyncMinMessageSizeRequest,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, min_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     6,
                     SyncRecvComplete,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, received_bytes))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     7,
                     SyncSendComplete,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     8,
                     AsyncDispatchRequest,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request_type))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     9,
                     AsyncDispatchRecv,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, device),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, irq_enable_status))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     10,
                     AsyncDispatchSend,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, device))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     11,
                     DeviceInitInvalidateRecvChannel,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, device),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_addr),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_size))


FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     12,
                     DeviceInitInvalidateSendChannel,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, device),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_addr),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, mem_size))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     13,
                     DeviceInitRecvReset,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, device))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     14,
                     InterfaceInit,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, device))

FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     15,
                     DispatcherMatchCb,
                     FPFW_ET_LEVEL_VERBOSE,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, client_msg),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, entry_msg))

//
// WARNING Events - Offset by 100 to Event Id
//
FPFW_ET_DEFINE_EVENT(EVENT_TRACE_PROVIDER_ID_COMMON_MHU_ICC_TRANSPORT,
                     100,
                     SyncRecvUnfilledPacketSize,
                     FPFW_ET_LEVEL_WARNING,
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, request),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, buffer_size),
                     FPFW_ET_DEFINE_FIELD(FPFW_ET_UINT32_HEX, new_payload_size))
