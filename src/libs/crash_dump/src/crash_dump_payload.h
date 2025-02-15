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

/*-- Symbolic Constant Macros (defines) --*/

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
 * #param type_context crash dump type based context
 */
void crash_dump_register_core_registers(crash_dump_type_context_t *type_context);

/**
 * @brief Captures default crash registers
 * 
 * #param type_context crash dump type based context
 * @param mmio_registers Array of core_register_mmio_t
 * @param mmio_register_count Number of elements in mmio_registers
 */
void crash_dump_register_default_registers(crash_dump_type_context_t *type_context, const core_register_mmio_t *mmio_registers, uint32_t mmio_register_count);

/**
 * @brief Captures core stack
 * 
 * #param type_context crash dump type based context
 */
void crash_dump_register_core_stack(crash_dump_type_context_t *type_context);

/**
 * @brief Captures standard information
 * 
 * #param type_context crash dump type based context
 */
void crash_dump_register_standard_info(crash_dump_type_context_t *type_context);

/**
 * @brief Register callback to capture ThreadX crash information to the crash dump.
 *
 * #param type_context crash dump type based context
 */
void crash_dump_register_threadx(crash_dump_type_context_t *type_context);

/**
 * @brief Captures threadX data
 *
 * @param context callback context
 */
void crash_dump_capture_threadx(void* context);
