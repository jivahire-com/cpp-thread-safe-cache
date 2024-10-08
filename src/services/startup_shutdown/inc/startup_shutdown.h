// Copyright Microsoft. All rights reserved.

/**
 * @file startup_shutdown.h
 * This file contains the interface to the startup/shutdown service
 */

#pragma once

/*----------- Nested includes ------------*/

#include "startup_shutdown_ssi.h"

#include <DfwkCommon.h>
#include <FpFwAssert.h>
#include <FpFwLinkedList.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

/**
 *  Synchonous and Asynchronous Interface Request IDs
 */
enum STARTUP_SHUTDOWN_REQUEST_IDS
{
    STARTUP_REGISTER_SSI_SYNC,
    STARTUP_RESET_TIMEOUT_SYNC,
    STARTUP_REQUEST_START_PHASE_SYNC,
    STARTUP_REQUEST_SHUTDOWN_ASYNC,
    STARTUP_REQUEST_START_PHASE_ASYNC,
};

typedef struct _startup_ssi_registration_t
{
    FPFW_LIST_ENTRY list_entry;
    ssi_request_t ssi_request; // we need a request per-registration, so define it here
    PDFWK_INTERFACE_HEADER p_ssi_interface;
    uint32_t interface_unique_flag;
} startup_ssi_registration_t, *pstartup_ssi_registration_t;

typedef struct _startup_register_ssi_request
{
    DFWK_SYNC_REQUEST_HEADER header;
    pstartup_ssi_registration_t p_registration; // pointer to registration which should remain available after init (static)
} startup_register_ssi_request, *pstartup_register_ssi_request;

typedef struct _startup_reset_timeout_request_t
{
    DFWK_SYNC_REQUEST_HEADER header;
    uint32_t timeout_ms;
} startup_reset_timeout_request_t, *pstartup_reset_timeout_request_t;

typedef struct _startup_start_phase_request_t
{
    union
    {
        DFWK_SYNC_REQUEST_HEADER sync;
        DFWK_ASYNC_REQUEST_HEADER async;
    } header;
    ssi_startup_type_t boot_type;
    ssi_startup_stage_t stage;
} startup_start_phase_request_t, *pstartup_start_phase_request_t;

typedef struct _startup_shutdown_request_t
{
    DFWK_ASYNC_REQUEST_HEADER header;
    ssi_shutdown_type_t shutdown_type;
} startup_shutdown_request_t, *pstartup_shutdown_request_t;

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/

/**
 *  Function to register a startup/shutdown interface to the startup service
 *
 *  @param p_interface The interface to the startup service
 *  @param p_registration Pointer to a statically defined registration object (service will use this to track a list of registrations)
 *  @param p_ssi_interface An interface to a driver that implements the startup/shutdown interface.
 *
 *  @return true if supported
 */
int32_t sos_register_ssi(PDFWK_INTERFACE_HEADER p_interface,
                         pstartup_ssi_registration_t p_registration,
                         PDFWK_INTERFACE_HEADER p_ssi_interface);

/**
 *  Function to request reset of the current stage's timeout.   This can be useful for overriding
 *  a default timeout AND for resetting the timeout in the case of retry.  The service will only increase
 *  current timeout, so timeout's shorter than remaining will be ignored.
 *
 *  @param p_interface The interface to the startup service
 *  @param timeout The updated timeout.
 *
 *  @return true if supported
 */
int32_t sos_reset_timeout(PDFWK_INTERFACE_HEADER p_interface, uint32_t timeout);

/**
 *  Function to send a synchronous or asynchronous phase start request
 *
 *  @param p_interface The interface to the startup service
 *  @param p_request A DFWK request with enough space for a startup_shutdown_request_t or NULL for synchronous
 *  @param boot_type The type of boottype being requested
 *  @param startup_stage The stage to start
 *  @param completion_routine A completion routine to call when the operation has completed or NULL for synchronous.
 *  @param p_completion_context A context that is supplied when the completion routine is called.
 */
void sos_start_phase(PDFWK_INTERFACE_HEADER p_interface,
                     pstartup_start_phase_request_t p_request,
                     ssi_startup_type_t boot_type,
                     ssi_startup_stage_t startup_stage,
                     DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                     void* p_completion_context);

/**
 *  Function to send an asynchronous shutdown request
 *
 *  @param p_interface The interface to the startup service
 *  @param p_request A DFWK request with enough space for a startup_shutdown_request_t
 *  @param shutdown_type The type of shutdown being requested
 *  @param completion_routine A completion routine to call when the operation has completed.
 *  @param p_completion_context A context that is supplied when the completion routine is called.
 */
void sos_shutdown(PDFWK_INTERFACE_HEADER p_interface,
                  pstartup_shutdown_request_t p_request,
                  ssi_shutdown_type_t shutdown_type,
                  DFWK_ASYNC_REQUEST_COMPLETION_ROUTINE completion_routine,
                  void* p_completion_context);
