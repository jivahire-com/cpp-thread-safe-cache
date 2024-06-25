//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file crash_dump.h
 *  Public API and types for crash dump
 */

#pragma once

/*--------------- Includes ---------------*/
#include <FpFwUtils.h>
#include <modules/CdDumpManager.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef struct _CD_GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} CD_GUID;

typedef struct __attribute__((__packed__)) {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
} core_crash_context_t;

/*-- Declarations (Statics and globals) --*/
extern core_crash_context_t g_core_crash_context;

/*--------- Function Prototypes ----------*/
/**
 * @brief Initiates a bug check which will create a crash dump.
 *
 * @param errorCode User defined error code stored with the crash dump bug check details
 *
 * @param p1, p2, p3, p4
 *  User defined parameter data stored with the crash dump bug check details
 */
FPFW_NORETURN void crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4);

/**
 * @brief Get the Crash Dump Context object
 * 
 * @return Pointer to static crash dump context.
 */
FPFwCrashDumpCtx *GetCrashDumpContext();

/**
 * @brief Initialize crash dump components
 * 
 */
void crash_dump_init();

/**
 * @brief Crash dump handler handles failure exceptions and generates a crash dump
 * 
 * @param errorCode User defined error code stored with the crash dump bug check details
 *
 * @param p1, p2, p3, p4
 *  User defined parameter data stored with the crash dump bug check details
 */
void crash_dump_handler(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4);

/**
 *
 * @brief a callback to be run at crash time, prior to beginning crash dump
 *
 * @param cb User defined callback function
 *
 * @param ctx User defined callback function context (supplied to callback when called)
 */
void crash_dump_register_pre_dump_callback(void cb(void *), void *ctx);

/**
 *
 * @brief Registers a set of MMIO registers to be recorded in the crash dump
 *
 * @param mmio_reg MMIO register address
 *
 * @param reg_count Number of registers to capture
 *
 * @param priority One of FPFW_CD_DUMP_PRIORITY_CRITICAL, FPFW_CD_DUMP_PRIORITY_NORMAL, FPFW_CD_DUMP_PRIORITY_OPPORTUNISTIC
 *                 Data is stored in the dump in the above priority order until the dump memory is exhausted
 */
void crash_dump_register_mmio_register(volatile void* mmio_reg, uint32_t reg_count, FPFwCdDumpPriority priority);

/**
 *
 * @brief Registers a region of memory to be recorded in the crash dump
 *
 * @param address Pointer to memory
 *
 * @param size Size of memory
 *
 * @param priority One of FPFW_CD_DUMP_PRIORITY_CRITICAL, FPFW_CD_DUMP_PRIORITY_NORMAL, FPFW_CD_DUMP_PRIORITY_OPPORTUNISTIC
 *                 Data is stored in the dump in the above priority order until the dump memory is exhausted
 */
void crash_dump_register_address32(void* address, uint32_t size, FPFwCdDumpPriority priority);

/**
 *
 * @brief Registers a region of memory to be recorded in the crash dump
 *
 * @param address Address of memory
 *
 * @param size Size of memory
 *
 * @param priority One of FPFW_CD_DUMP_PRIORITY_CRITICAL, FPFW_CD_DUMP_PRIORITY_NORMAL, FPFW_CD_DUMP_PRIORITY_OPPORTUNISTIC
 *                 Data is stored in the dump in the above priority order until the dump memory is exhausted
 */
void crash_dump_register_address64(uint64_t address, uint32_t size, FPFwCdDumpPriority priority);

/**
 * @brief Registers an array of 32 bit addresses to store during a crash dump
 * 
 * @param priority One of FPFW_CD_DUMP_PRIORITY_CRITICAL, FPFW_CD_DUMP_PRIORITY_NORMAL, FPFW_CD_DUMP_PRIORITY_OPPORTUNISTIC
 *                 Data is stored in the dump in the above priority order until the dump memory is exhausted
 * @param minChunkSize Minimum size of chunks to be stored
 * @param maxRegistrationCount Maximum number of addresses to register
 * @param pointerArray Array of 32 bit addresses to store
 * @param pointerArrayCount Number of addresses in pointerArray
 */
void crash_dump_register_address32_pointer_array(FPFwCdDumpPriority priority, uint32_t minChunkSize, uint32_t maxRegistrationCount, void** pointerArray, uint32_t pointerArrayCount);
