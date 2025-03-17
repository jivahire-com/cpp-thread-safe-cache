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
#include <accelip_id.h>
#include <fpfw_icc_base.h>
#include <kng_error.h>
#include <modules/CdDumpDescriptor.h>
#include <modules/CdDumpManager.h>
#include <semaphore_lib.h>
#include <stdint.h>
#include <tx_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-- Symbolic Constant Macros (defines) --*/
#define CRASH_DUMP_NUM_DESCRIPTORS 200 // ToDo: Re-evaluate this number

#define CRASH_DUMP_PROCESSOR_ID(d, c)   (((d) << 16) | ((c) & 0xFFFF))

#define PRE_DUMP_CB_MAX  8
#define POST_DUMP_CB_MAX 8

/*-------------- Typedefs ----------------*/
typedef enum
{
    CRASH_DUMP_CORE_MCP = 0,
    CRASH_DUMP_CORE_SCP = 1,
    CRASH_DUMP_CORE_HSP = 2,
    CRASH_DUMP_CORE_SDM = 3,
    CRASH_DUMP_CORE_CDED = 4,
    CRASH_DUMP_CORE_KMP = 5,
    CRASH_DUMP_CORE_NUM
} crash_dump_core_t;

typedef enum
{
    CRASH_DUMP_TYPE_ALL = -1,
    CRASH_DUMP_TYPE_MINI = 0,
    CRASH_DUMP_TYPE_FULL = 1,
    CRASH_DUMP_TYPE_NUM
} crash_dump_type_t;

typedef enum : uint16_t
{
    CRASH_DUMP_NOT_IN_USE = 0,
    CRASH_DUMP_IN_USE = 1
} crash_dump_state_t;

typedef enum : uint8_t
{
    CRASH_DUMP_STATE_NOT_AVAILABLE = 0,
    CRASH_DUMP_STATE_READY = 1,
    CRASH_DUMP_STATE_IN_PROGRESS = 2,
    CRASH_DUMP_STATE_COMPLETED = 3
} crash_dump_core_state_t;

typedef enum
{
    CRASH_DUMP_ICC_CONFIG_MHU_LOCAL = 0,
    CRASH_DUMP_ICC_CONFIG_MHU_REMOTE = 1,
    CRASH_DUMP_ICC_CONFIG_SPI_REMOTE = 2,
    CRASH_DUMP_ICC_CONFIG_HSP = 3,
    CRASH_DUMP_ICC_CONFIG_SDM = 4,
    CRASH_DUMP_ICC_CONFIG_CDED = 5,
    CRASH_DUMP_ICC_CONFIG_MAX
} crash_dump_icc_config_t;

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

typedef struct {
    volatile void *address;
    uint32_t count;
    FPFwCdDumpPriority priority;
} core_register_mmio_t;

/**
 * @brief Crash dump status header type
 * 
 * cd_status: Crash dump status (Lower 8 bits: DIE0, Upper 8 bits: DIE1 - 0: In progress, 1: Completed for each core)
 * 
 */
typedef struct {
    volatile uint16_t status; // 0: Not in use, 1: In Use
    volatile uint8_t cores[CRASH_DUMP_CORE_NUM * 2];
} crash_dump_header_t;

/**
 * @brief Crashdump callback type
 * 
 */
typedef struct
{
    void (*callback_fn)(void*);
    void* callback_ctx;
} crash_dump_callback_t;

typedef struct
{
    crash_dump_callback_t pre_dump_callbacks[PRE_DUMP_CB_MAX];
    uint32_t pre_dump_cb_count;

    crash_dump_callback_t post_dump_callbacks[POST_DUMP_CB_MAX];
    uint32_t post_dump_cb_count;
} crash_dump_type_callback_t;


/**
 * @brief HW semaphore configuration
 * 
 */
typedef struct {
    SEMAPHORE_ID id;
    uint32_t key;
} crash_dump_semaphore_t;

/**
 * @brief Crash dump type based context (mini or full)
 * 
 */
typedef struct {
    crash_dump_type_t type;
    uint64_t mem_pool_addr;
    uint32_t mem_pool_size;

    crash_dump_semaphore_t semaphore;    // Semaphore for header access
    crash_dump_header_t *header;         // Crash dump status header

    // FPFW Crash dump context
    FPFwCrashDumpCtx crash_dump_ctx;
    FPFwCDMemPoolCtx mem_ctx;
    FPFwCDDumpDescriptorCtx desc_ctx;
    FPFwCDDumpFileCtx file_ctx;
    FPFwCDDumpDescriptor desc_list[CRASH_DUMP_NUM_DESCRIPTORS];
    TX_MUTEX desc_mutex;
} crash_dump_type_context_t;

typedef struct {
    uint32_t cd_file_offset;
    uint32_t cd_file_size;
    uint32_t cd_magic_nr_offset;
} crash_dump_accel_context_t;

/**
 * @brief Crash dump context type
 * 
 */
typedef struct {
    //
    // Per type contexts
    //
    crash_dump_type_context_t *type_ctx[CRASH_DUMP_TYPE_NUM];   // type contexts.
    crash_dump_type_callback_t callbacks[CRASH_DUMP_TYPE_NUM];  // crashdump callbacks
    //
    // Global context
    //
    uint32_t die_index;                                         // DIE index
    uint32_t core_index;                                        // Core index
    bool is_primary;
    bool single_core_mode;
    fpfw_icc_base_ctx_t *icc_ctx[CRASH_DUMP_ICC_CONFIG_MAX];    // ICC context

    // Core dependent descriptor registrations
    uint32_t mmio_register_count;
    const core_register_mmio_t *mmio_registers;

    // Core dependent override functions
    bool (*in_memory)(uintptr_t start_addr, uintptr_t end_addr);
	
	// Accelerators
    crash_dump_accel_context_t accel_cd_ctx[NUM_VALID_ACCEL_ID];
} crash_dump_context_t;

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
 * @brief Initiates a crash dump creation signalled by external core.
 */
FPFW_NORETURN void crash_dump_bug_check_external();

/**
 * @brief Check if a bug check has been initiated
 *
 * @return true if a bug check has been initiated
 */
bool crash_dump_bug_check_initiated_dump();

/**
 * @brief Wait forever
 */
FPFW_NORETURN void crash_dump_wait_forever();

/**
 * @brief Get the Crash Dump Context object
 * 
 * @return Pointer to static crash dump context.
 */
crash_dump_context_t *crash_dump_context();

/**
 * @brief Initialize crash dump components
 * 
 */
void crash_dump_init(crash_dump_context_t *context);

/**
 * @brief Register mini dump or full dump
 * 
 * @param type_context Crash dump context applied to specific type (mini or full)
 * @return KNG_SUCCESS if succeeded, otherwise error code.
 */
KNG_STATUS crash_dump_register_dump(crash_dump_type_context_t *type_context);

/**
 * @brief Configure ICC context for crash dump
 * 
 * @param type ICC configuration type
 * @param icc_ctx ICC context
 */
void crash_dump_config_icc(crash_dump_icc_config_t type, fpfw_icc_base_ctx_t *icc_ctx);

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
void crash_dump_register_pre_dump_callback(void cb(void *), void *ctx, crash_dump_type_t dump_type);

/**
 *
 * @brief a callback to be run after crash dump is collected
 *
 * @param cb User defined callback function
 *
 * @param ctx User defined callback function context (supplied to callback when called)
 */
void crash_dump_register_post_dump_callback(void cb(void*), void* ctx, crash_dump_type_t dump_type);

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

/**
 * @brief Check crash dump is completed
 * 
 * @param type_context Crash dump context applied to specific type (mini or full)
 *                     If NULL, check all types of crash dump.
 * @return true if all expected cores dumps are completed, otherwise false.
 */
bool crash_dump_get_is_dump_complete(crash_dump_type_context_t* type_context);

/**
 * @brief Registers crash dump CLI commands
 * 
 */
void crash_dump_cli_init(void);

/**
 * @brief Captures Accel device externally accessible registers
 * 
 */
void crash_dump_register_accel_ext_mmio();

/**
 * @brief Check is given accel core has completed its CD file collection
 * 
 * @param[in] accel_type SDM or CDED accel type
 * @return true - CD file collection complete. false - CD file collection not complete
 */
bool crash_dump_is_accel_cd_complete(ACCEL_ID accel_type);

#ifdef __cplusplus
}
#endif
