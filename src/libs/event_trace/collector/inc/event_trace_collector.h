//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file event_trace_collector.h
 *  Defines public types and methods for the event trace collector library.
 */

#pragma once

/*--------------- Includes ---------------*/

#include <stddef.h>
#include <stdint.h>

#include <IFpFwEventTracingBuffers.h>
#include <IFpFwEventTracingController.h>
#include <IFpFwEventTracingStatus.h>
#include <tx_api.h>

/*-- Symbolic Constant Macros (defines) --*/

#define ETC_SERVICE_EVENT_QUOTA (30)
#define ETC_SERVICE_CORE_BUFFER_COUNT (FPFW_ET_TRACE_BUFFER_COUNT)

#define ETC_WORK_QUEUE_NAME ("etc-work_queue")
#define ETC_EVENT_QUOTA_SEM_NAME ("etc-event_quota")
#define ETC_WORKER_THREAD_NAME ("etc-worker_thread")

/*-------------- Typedefs ----------------*/

typedef enum 
{
    ETC_SERVICE_MODE_STDIO,
    ETC_SERVICE_MODE_MAX
} ETC_SERVICE_MODE;

typedef enum
{
    ETC_SERVICE_REQUEST_TYPE_PROCESS_CORE_BUFFER,
    ETC_SERVICE_REQUEST_TYPE_PROCESS_EVENT
} ETC_SERVICE_REQUEST_TYPE;

typedef struct
{
    ETC_SERVICE_REQUEST_TYPE type;
    union
    {
        struct
        {
            PFPFW_ET_CORE_BUFFER_HEADER p_core_buffer;
            PFPFW_ET_CONTROLLER p_origin_controller;
        } core_buffer_request;
        struct
        {
            void* p_event;
        } event_request;
    };
} etc_service_completion_request_t;

typedef void (*ETC_PROCESS_REQUEST_FUNC)(void*, etc_service_completion_request_t*);

typedef struct {
    void* p_context;               // Context to pass to processing function
    ETC_PROCESS_REQUEST_FUNC func; // Request processing function
} process_request_t;

typedef struct 
{
    TX_THREAD worker_thread;
    TX_QUEUE work_queue;
    TX_SEMAPHORE event_quota;
    FPFW_ET_CONTROLLER controller;
    process_request_t process_request;
    etc_service_completion_request_t request_pool[ETC_SERVICE_EVENT_QUOTA + ETC_SERVICE_CORE_BUFFER_COUNT]; // Quota + 2 core buffers
} etc_service_context_t;

// clang-format off
typedef struct
{
    ETC_SERVICE_MODE mode;            // Operational mode for event tracing (for more info see design doc)
    uint32_t core_id;                 // Core id to be used to tag the traces
    FPFW_ET_MANIFEST_ID manifest_id;  // Identifier to tag the payload buffers against a manifest file
                                      // this is usually the elf version id

    struct 
    {
        void* p_pool;       // Memory to be used to store ping/pong core trace buffers
        size_t byte_count;  // Size of the memory
    } trace_buffer_memory;
    struct 
    {
        void* p_stack;              // Memory used to the worker thread stack
        size_t stack_size;          // Size of the provided memory
        uint32_t priority;          // ThreadX Thread priority
        uint32_t time_slice_option; // ThreadX Thread slicing option
    } thread_config;
    struct
    {
        uintptr_t provider_start;  // Pointer to the start of the provider metadata
        uintptr_t provider_end;    // Pointer to the end of the provider metadata
        uintptr_t event_start;     // Pointer to the start of the event metadata
        uintptr_t event_end;       // Pointer to the end of the event metadata
    } event_manifest;              // Metadata used for decoding usually stored in the elf .Event/ProviderMetadata.msdata section
    struct
    {
        uintptr_t start;  // Pointer to the start of the filter array for the providers
        uintptr_t end;    // Pointer to the end of hte filter array for the providers
    } provider_filters;   // Filter array used to store the filter masks for the events usually stored in the elf .data section
} etc_service_config_t;
// clang-format on

/*--------- Function Prototypes ----------*/

/**
 *  @brief     Initializes core event trace processing requests for a core. Input configuration contains worker thread
 *             for core specific request handling.
 * 
 *  @param[in]  p_service    This is the service context where the internal state will be stored for the module
 *                           NOTE! The provided memory is used only for lifetime of the program
 * 
 *  @param[in]  p_config     This is the user configuration. Parameters are described in the documentation of the struct.
 *                           NOTE! The provided memory is used only for the lifetime of this function call.
 * 
 *  @retval     None. This will attempt to create the threadx resources and configure the trace controller. If these
 *              operations fail, the module will raise an error.
*/
void etc_initialize(etc_service_context_t* p_service, const etc_service_config_t* p_config);
