// Copyright Microsoft. All rights reserved.

/**
 * @file startup_shutdown_i.h
 * This file contains the internal definitions for the startup or shutdown service
 */

#pragma once

/*----------- Nested includes ------------*/

#include "startup_shutdown_ssi.h"

#include <DfwkCommon.h>
#include <FpFwAssert.h>
#include <FpFwLinkedList.h>
#include <stdint.h>
#include <stdio.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/
/*-- Symbolic Constant Macros (defines) --*/
#define MODULE_NAME "[SoS] "
#define NEWLINE     "\n"

// set to 1 for more verbose logs
#define SOS_ENABLE_TRACE_LEVEL_LOG 0

#if SOS_ENABLE_TRACE_LEVEL_LOG
#define SOS_LOG_TRACE(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#else
#define SOS_LOG_TRACE(fmt, ...) 
#endif
#define SOS_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define SOS_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define SOS_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

/*-------------- Typedefs ----------------*/
// struct for queue entry
enum sos_queue_entry_type_t
{
    SOS_QUEUE_ENTRY_TYPE_BOOT_PHASE,
    SOS_QUEUE_ENTRY_TYPE_SHUTDOWN
};

typedef struct _ssi_startup_phase_t
{
    ssi_startup_stage_t phase;
    ssi_startup_type_t boot_type;
} ssi_startup_phase_t;

typedef struct
{
    enum sos_queue_entry_type_t type; // type of entry
    union
    {
        ssi_startup_phase_t boot_phase;
        ssi_shutdown_type_t shutdown_type;
    } data;
    PDFWK_ASYNC_REQUEST_HEADER p_request; // pointer to request; may be null
} sos_queue_entry_t;

typedef struct
{
    FPFW_LIST_HANDLE ssi_registrations;
    uint32_t registration_count;
} sos_service_context_t, *psos_service_context_t;

/**
 *  Structure for defining table of boot stages
 */
typedef struct _startup_shutdown_boot_stage_t
{
    ssi_startup_stage_t phase;
    ssi_startup_stage_t stage;
    unsigned timeout_ms;
    bool local_core_sync_required; // placeholder for synchronization across local cores
    bool remote_die_sync_required; // placeholder for synchronization across dies (TBD: if local core sync and remote die sync are both required, expect remote die handles local core sync before acking die sync?)
} startup_shutdown_boot_stage_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/
unsigned sos_core_boot_stage_count();
const startup_shutdown_boot_stage_t* sos_core_boot_stages();
void sos_core_shutdown_handler(ssi_shutdown_type_t shutdown_type);

// interface to start a phase
void sos_queue_start_phase(ssi_startup_type_t boot_type, ssi_startup_stage_t phase, PDFWK_ASYNC_REQUEST_HEADER p_request);
void sos_queue_shutdown(ssi_shutdown_type_t shutdown_type, PDFWK_ASYNC_REQUEST_HEADER p_request);
void sos_thread_init(psos_service_context_t p_context);

// internal interfaces - declared here for test
int sos_queue_find_phase(ssi_startup_stage_t phase);
void sos_completion(PDFWK_ASYNC_REQUEST_HEADER request, void* p_completion_context);
void sos_notify_ssi_boot_stage(psos_service_context_t p_context, ssi_startup_stage_t stage, ssi_startup_type_t startup_type, bool start);
void sos_notify_ssi_shutdown(psos_service_context_t p_context, ssi_shutdown_type_t shutdown_type);
void wait_ssi_complete();
void sos_notify_ssi_boot_stage_and_wait(psos_service_context_t p_context,
                                        ssi_startup_stage_t stage,
                                        ssi_startup_type_t startup_type,
                                        bool start);

