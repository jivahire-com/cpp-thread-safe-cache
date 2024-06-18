//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_payload.h
 * APIs and types for registering crash dump payload to the crash dump
 */
#pragma once

/*---------- Nested Includes -------------*/
#include <build_data.h> // for BUILD_ELF_SECTION_BINARY_METADATA
#include <crash_dump.h> // for CD_GUID
#include <tx_api.h>     // for ULONG, TX_THREAD

/*-- Symbolic Constant Macros (defines) --*/
#define SHA1_GNU_ELF_BUILD_ID_SIZE__bytes 20

/*------------- Typedefs -----------------*/
typedef struct _CD_THREADX_DATA
{
    CD_GUID guid;
    void* _tx_thread_system_stack_ptr;
    TX_THREAD* _tx_thread_current_ptr;
    TX_THREAD* _tx_thread_execute_ptr;
    TX_THREAD* _tx_thread_created_ptr;
    ULONG _tx_thread_created_count;
    ULONG _tx_thread_system_state;
} CD_THREADX_DATA;

typedef struct _GNU_BUILD_ID
{
    uint32_t nameSize;
    uint32_t buildIdSize;
    uint32_t type;
    uint32_t name;
    uint8_t buildId[SHA1_GNU_ELF_BUILD_ID_SIZE__bytes];
} GNU_BUILD_ID;

typedef struct _CD_MSFT_VERSION_INFO
{
    CD_GUID guid;
    BUILD_ELF_SECTION_BINARY_METADATA versionInfo;
    uint8_t elf_build_id[SHA1_GNU_ELF_BUILD_ID_SIZE__bytes];
} CD_MSFT_VERSION_INFO;

/*-------- Function Prototypes -----------*/
/**
 * @brief Captures built-in Cortex M7 registers
 * 
 */
void crash_dump_register_core_registers();

/**
 * @brief Captures default crash registers
 * 
 */
void crash_dump_register_default_registers();

/**
 * @brief Captures core stack
 * 
 */
void crash_dump_register_core_stack();

/**
 * @brief Captures standard information
 * 
 */
void crash_dump_register_standard_info();

/**
 * @brief Register callback to capture ThreadX crash information to the crash dump.
 *
 */
void crash_dump_register_threadx();

/**
 * @brief Captures threadX data
 * 
 * @param context callback context
 */
void crash_dump_capture_threadx(void* context);
