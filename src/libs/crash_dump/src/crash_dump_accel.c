//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_accel.c
 * Registering accel external registers data to put in crash dump
 */

/*--------------- Includes ---------------*/
#include "crash_dump_accel.h"

#include "crash_dump_icc.h"
#include "crash_dump_memory.h"
#include "crash_dump_overrides.h"
#include "crash_dump_status.h" // for crash_dump_update_accel_state

#include <CrashDump.h>           // for FPFwCDPrintf
#include <DbgPrint.h>            // for FPFW_DBGPRINT_INFO
#include <FpFwUtils.h>           // for FPFW_MIN
#include <accel_intr.h>          // for accel_intr_has_accel_crashed
#include <accel_intr_virt_irq.h> // for accel_intr_get_isr_value
#include <atu_init.h>            // for atu_svc_accel_atu_addr
#include <bug_check.h>           // for BUG_ASSERT_PARAM
#include <cded_regs_regs.h>      // for CDED_REGS_CCMP_CFG_ADDRESS
#include <cdedss_config_regs.h>  // for CDEDSS_CONFIG_CDED_REGS_REGS_ADDRESS
#include <crash_dump.h>          // for crash_dump_register_address32
#include <crash_dump_events.h>   // for CRASH_DUMP_ET
#include <gtimer_prodfw.h>       // for gtimer_get_timestamp_ms
#include <health_monitor.h>      // for hm_send_accel_error_cper
#include <idsw.h>                // for idsw_get_cpu_type, idsw_get_die_id
#include <idsw_kng.h>
#include <kng_error.h>
#include <sdm_ext_cfg_regs.h>        // for SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS...
#include <silibs_common.h>           // for BYTES_IN_WORD32
#include <silibs_platform.h>         // for MMIO_READ32
#include <stdint.h>                  // for uint32_t
#include <string.h>                  // for memcpy
#include <utc_sync_client_service.h> // for utc_sync_client_get_current_time_epoch_ms

/**
 * All SDM external config registers are offset by 0x100000
 * in the generate header file
 */
#define ADDRESS_BLOCK_0x100000_OFFSET (SDM_EXT_CFG__ADDRESSBLOCK_0X100000_ADDRESS)

/**
 * CD magic number which indicates that CD collection is complete
 *
 */
#define CD_MAGIC_NUMBER 0xCDED5D55

#define CORE_BUILTIN_REG_INDEX 0

#define CRASH_DUMP_MAX_RETRY_COUNT               2
#define CRASH_DUMP_MAX_RETRY_DELAY_US            2000 // 2 seconds
#define CRASH_DUMP_DEFAULT_ACCEL_NUM_DESCRIPTORS 1    // Need 1 descriptor for the register info

/*-------------- Typedefs ----------------*/

typedef struct
{
    uint64_t mem_pool_addr;
    uint32_t mem_pool_size;

    FPFwCrashDumpCtx crash_dump_ctx;
    FPFwCDMemPoolCtx mem_ctx;
    FPFwCDDumpDescriptorCtx desc_ctx;
    FPFwCDDumpFileCtx file_ctx;
    FPFwCDDumpDescriptor desc_list[CRASH_DUMP_DEFAULT_ACCEL_NUM_DESCRIPTORS];

    TX_MUTEX desc_mutex;
} crash_dump_accel_ctx_t;

/*-- Declarations (Statics and globals) --*/

static uint8_t s_retry_count = 0;
static crash_dump_accel_ctx_t s_crash_dump_accel_ctx[NUM_VALID_ACCEL_ID];
core_crash_context_t g_accel_core_crash_context[NUM_VALID_ACCEL_ID];

/*--------- Private Function ----------*/

static bool crash_dump_wait_for_accel_cd_complete(ACCEL_ID accel_type)
{
    /**
     * CD collection of SDM and CDED happens in parallel. Wait only for the first
     * accel core to complete the crash dump collection. The later accel core
     * should have completed the crash dump collection by the time we reach here.
     */
    if (accel_intr_get_cd_skip(accel_type))
    {
        CRASH_DUMP_ET_INFO_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_CD_SKIP, accel_type);
        crash_dump_generate_default_accel_cd(accel_type);
        return false;
    }

    CRASH_DUMP_ET_INFO_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_CD_WAIT_START, accel_type);
    while (crash_dump_is_accel_cd_complete(accel_type) == false)
    {
        if (s_retry_count >= CRASH_DUMP_MAX_RETRY_COUNT)
        {
            CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_CD_WAIT_FAILED, accel_type);
            crash_dump_generate_default_accel_cd(accel_type);
            return false;
        }

        s_retry_count++;
        // Allow some time for the accel core to complete the crash dump collection
        SLEEP_US(CRASH_DUMP_MAX_RETRY_DELAY_US);
    }
    CRASH_DUMP_ET_INFO_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_CD_WAIT_DONE, accel_type);

    return true;
}

static void convert_soc_time_in_accel_cd_to_utc(void* cd_base_addr)
{
    /**
     * The CrashDump library generates a binary file with a hierarchical structure consisting of three main
     * sections: Header, Metadata, and Payload. The bCD Binary from the accelerator has the following layout:
     *
     * ┌─────────────────────────────────────────────────────────────────┐
     * │                        CRASH DUMP FILE                          │
     * ├─────────────────────────────────────────────────────────────────┤
     * │  OFFSET 0                                                       │
     * │  ┌───────────────────────────────────────────────────────────┐  │
     * │  │                     DUMP_HEADER                           │  │
     * │  │  • Magic: DUMP_HEADER_MAGIC_COMPLETE (when finalized)     │  │
     * │  │  • Version: DUMP_HEADER_VERSION                           │  │
     * │  │  • FileSize: Total size of dump file                      │  │
     * │  │  • MetadataExtent: { Offset, Size }                       │  │
     * │  │  • PayloadExtent: { Offset, Size }                        │  │
     * │  └───────────────────────────────────────────────────────────┘  │
     * ├─────────────────────────────────────────────────────────────────┤
     * │  METADATA OFFSET (dump_header_size)                             │
     * │  ┌───────────────────────────────────────────────────────────┐  │
     * │  │                  DUMP_METADATA_HEADER                     │  │
     * │  │  • Magic: DUMP_METADATA_MAGIC                             │  │
     * │  │  • Version: DUMP_METADATA_VERSION                         │  │
     * │  │  • Product: Product ID (e.g., Athena, Braga, Pioneer)     │  │
     * │  │  • Type: DumpType (Mini, Full, etc.)                      │  │
     * │  │  • Root: DUMP_CHUNK_DESCRIPTOR (Type=Aggregate)           │  │
     * │  │      └─ ChunkCount: Number of metadata chunks             │  │
     * │  └───────────────────────────────────────────────────────────┘  │
     * ├─────────────────────────────────────────────────────────────────┤
     * │  PAYLOAD OFFSET (dump_header_size + metadata_size)              │
     * │  ┌───────────────────────────────────────────────────────────┐  │
     * │  │                  DUMP_PAYLOAD_HEADER                      │  │
     * │  │  • Magic: DUMP_PAYLOAD_MAGIC                              │  │
     * │  │  • Version: DUMP_PAYLOAD_VERSION                          │  │
     * │  │  • Configuration: Configuration ID                        │  │
     * │  │  • Type: DumpType                                         │  │
     * │  │  • Root: DUMP_CHUNK_DESCRIPTOR (Type=Aggregate)           │  │
     * │  │      └─ ChunkCount: Number of payload chunks              │  │
     * │  └───────────────────────────────────────────────────────────┘  │
     * │                                                                 │
     * │  ┌───────────────────────────────────────────────────────────┐  │
     * │  │              DUMP_AGGREGATE_GENERIC (Root)                │  │
     * │  │  • AggregateDescriptor                                    │  │
     * │  │      └─ ChunkDescriptor (Type=Aggregate)                  │  │
     * │  │      └─ ProcessorId (PRID)                                │  │
     * │  │                                                           │  │
     * │  │  ┌─────────────────────────────────────────────────────┐  │  │
     * │  │  │            DUMP_AGGREGATE_GENERIC (Core)            │  │  │
     * │  │  │  Contains all chunks for one processor core         |  |  |
     * |  |  |  • AggregateDescriptor                              │  │  |
     * │  │  |   └─ ChunkDescriptor (Type=Aggregate)               │  │  |
     * │  │  |   └─ ProcessorId (PRID)                             │  │  │
     * │  │  │                                                     │  │  │
     * │  │  │  ┌───────────────────────────────────────────────┐  │  │  │
     * │  │  │  │     DUMP_CHUNK_CRASH_INFORMATION              │  │  │  │
     * │  │  │  │  • ChunkDescriptor (Type=CrashInformation)    │  │  │  │
     * │  │  │  │  • Tcon[2]: Crash timestamp (64-bit split)    │  │  │  │
     * │  │  │  │  • Bugcheck:                                  │  │  │  │
     * │  │  │  │      └─ Code: Bug check error code            │  │  │  │
     * │  │  │  │      └─ Parameter[4]: 4 parameters            │  │  │  │
     * │  │  │  └───────────────────────────────────────────────┘  │  │  │
     * │  │  │                                                     │  │  │
     * │  │  │                                                     │  │  │
     * │  │  │  ... (Memory chunks based on registered descriptors)│  │  │
     * │  │  └─────────────────────────────────────────────────────┘  │  │
     * │  │                                                           │  │
     * │  │  ... (Additional core aggregates for multi-core dumps)    │  │
     * │  └───────────────────────────────────────────────────────────┘  │
     * └─────────────────────────────────────────────────────────────────┘
     *
     *  Hence, the offset of the timestamp (Tcon) from the base address is:
     *  Tcon_offset = sizeof(DUMP_HEADER)
     *              + sizeof(DUMP_METADATA_HEADER)
     *              + sizeof(DUMP_PAYLOAD_HEADER)
     *              + sizeof(DUMP_AGGREGATE_GENERIC)      // Root aggregate
     *              + sizeof(DUMP_AGGREGATE_GENERIC)      // Core aggregate
     *              + sizeof(DUMP_CHUNK_DESCRIPTOR)       // CrashInformation header
     *
     * where
     * offsetof(DUMP_CHUNK_CRASH_INFORMATION, Tcon) = sizeof(DUMP_CHUNK_DESCRIPTOR)
     */

    uint32_t crash_info_offset = sizeof(DUMP_HEADER) + sizeof(DUMP_METADATA_HEADER) +
                                 sizeof(DUMP_PAYLOAD_HEADER) + sizeof(DUMP_AGGREGATE_GENERIC) // Root aggregate
                                 + sizeof(DUMP_AGGREGATE_GENERIC); // Core aggregate

    DUMP_CHUNK_CRASH_INFORMATION* p_dump_crash_info = (DUMP_CHUNK_CRASH_INFORMATION*)(cd_base_addr + crash_info_offset);

    uint64_t crash_time_ms = get_timestamp_from_cd(p_dump_crash_info);

    if (crash_time_ms != 0)
    {
        /* Fetch Current UTC Timestamp and current SoC timestamp in ms */
        uint64_t current_utc_time = utc_sync_client_get_current_time_epoch_ms();
        uint64_t current_system_time_ms = gtimer_get_timestamp_ms();

        /* Gate against UTC timestamp not available */
        if (current_utc_time != 0)
        {
            /* Compute and Update UTC Time at Crash */
            uint64_t utc_crash_time_ms = current_utc_time - (current_system_time_ms - crash_time_ms);
            update_timestamp_in_cd(p_dump_crash_info, utc_crash_time_ms);
        }
    }
}

static void copy_cd_file_dtcm_to_ddr(crash_dump_context_t* ctx, ACCEL_ID accel_type)
{
    uint32_t cd_ddr_addr;
    uint32_t accel_dtcm_addr;
    uint32_t cd_file_size = ctx->accel_cd_ctx[accel_type].cd_file_size;
    uint32_t cd_file_off = ctx->accel_cd_ctx[accel_type].cd_file_offset;
    char* accel_name = (accel_type == ACCEL_ID_SDM) ? "SDM" : "CDED";

    CRASH_DUMP_ET_INFO_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_DDR_COPY_START, accel_type);
    if (ctx->type_ctx[CRASH_DUMP_TYPE_FULL] == NULL)
    {
        // no full dump context.
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_ADDRESS, __LINE__);
        goto cd_transfer_failed;
    }

    if (cd_file_size == 0 || cd_file_off + cd_file_size > SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE)
    {
        // Invalid DTCM address or size.
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_SIZE, __LINE__);
        goto cd_transfer_failed;
    }

    if (accel_type == ACCEL_ID_SDM)
    {
        cd_ddr_addr = ctx->die_index == 0 ? CRASH_DUMP_FULL_SDM0_ADDR : CRASH_DUMP_FULL_SDM1_ADDR;
        cd_file_size = FPFW_MIN(CRASH_DUMP_FULL_SIZE_PER_CORE, cd_file_size);
    }
    else
    {
        cd_ddr_addr = ctx->die_index == 0 ? CRASH_DUMP_FULL_CDED0_ADDR : CRASH_DUMP_FULL_CDED1_ADDR;
        cd_file_size = FPFW_MIN(CRASH_DUMP_FULL_SIZE_PER_CORE, cd_file_size);
    }

    // Collect crash dump here only if the accel has not crashed
    if (!crash_dump_wait_for_accel_cd_complete(accel_type))
    {
        // Invalid MAGIC number address or CD collection is not complete for given accel core
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_CD_INCOMPLETE, __LINE__);
        cd_file_size = FPFwCDTotalMemoryAllocated(&s_crash_dump_accel_ctx[accel_type].mem_ctx);
        goto skip_cd_copy;
    }

    accel_dtcm_addr = atu_svc_accel_atu_addr(accel_type) + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS;
    memcpy((void*)cd_ddr_addr, (void*)(accel_dtcm_addr + cd_file_off), cd_file_size);

skip_cd_copy:
    // Transmit the cper to the OS at this point - this ensures we won't react to SCMI requests
    hm_send_accel_error_cper(accel_type);

    crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_COMPLETED);
    FPFW_DBGPRINT_INFO("%s:CD_DDR_ADDR:0x%x SZ:%d\r\n", accel_name, (int)cd_ddr_addr, (int)cd_file_size);

    /* Update timestamp to UTC from SoC timestamp*/
    convert_soc_time_in_accel_cd_to_utc((void*)cd_ddr_addr);

    return;

cd_transfer_failed:
    crash_dump_update_accel_state(accel_type, CRASH_DUMP_STATE_NOT_AVAILABLE);
    CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_TRANSFER_FAILED, __LINE__);
}

/**
 * @brief Initialize crash dump memory pool.
 *
 * @param cd_mem_pool Crash dump memory pool address.
 * @param block_size Size of the memory pool.
 *
 */
void init_mem_pool(crash_dump_accel_ctx_t* type_context)
{
    // Initialize crash dump memory pool
    BUG_ASSERT_PARAM(FPFwCDInitMemoryPool(&type_context->mem_ctx, type_context->mem_pool_addr, type_context->mem_pool_size),
                     type_context->mem_pool_addr,
                     type_context->mem_pool_size);
    (void)FPFwCDMemPoolOverrideCacheFlush(&type_context->mem_ctx, &cacheFlushOverride);
    (void)FPFwCDMemPoolOverrideCacheInvalidate(&type_context->mem_ctx, &cacheInvalidateOverride);
}

/**
 * @brief Initialize crash dump description.
 *
 */
void init_dump_desc(crash_dump_accel_ctx_t* type_context)
{
    // Create Tx mutex for descriptor set
    BUG_ASSERT(tx_mutex_create(&type_context->desc_mutex, "cd full mutex", TX_NO_INHERIT) == TX_SUCCESS);

    // Create crash dump descriptor
    BUG_ASSERT(FPFwCDInitDumpDescriptor(&type_context->desc_ctx, type_context->desc_list, CRASH_DUMP_DEFAULT_ACCEL_NUM_DESCRIPTORS));

    (void)FPFwCDDumpDescriptorSetMutexCtx(&type_context->desc_ctx, (void*)&type_context->desc_mutex);
    (void)FPFwCDDumpDescriptorOverrideMutexLock(&type_context->desc_ctx, &mutexLockOverride);
    (void)FPFwCDDumpDescriptorOverrideMutexUnlock(&type_context->desc_ctx, &mutexUnlockOverride);
}

/**
 * @brief Initialize crash dump file.
 *
 */
void init_dump_file(crash_dump_accel_ctx_t* type_context)
{
    BUG_ASSERT(FPFwCDInitDumpFile(&type_context->file_ctx));
    (void)FPFwCDDumpFileOverrideInValidMemory(&type_context->file_ctx, &inMemoryOverride);
    (void)FPFwCDDumpFileOverrideInValidCsrMemory(&type_context->file_ctx, &inMemoryOverride);
    (void)FPFwCDDumpFileOverrideInValidGlobalMemory(&type_context->file_ctx, &inGlobalMemoryOverride);
    type_context->file_ctx.product = CD_PRODUCT_ID_KINGSGATE;
}

/**
 * @brief Initialize crash dump manager.
 *
 */
void init_dump_manager(crash_dump_accel_ctx_t* type_context)
{
    BUG_ASSERT_PARAM(FPFwCDInitDumpManager(&type_context->crash_dump_ctx,
                                           &type_context->mem_ctx,
                                           &type_context->desc_ctx,
                                           &type_context->file_ctx,
                                           NULL, // No state manager
                                           type_context->mem_pool_size),
                     type_context->mem_pool_size,
                     0);
    FPFwCDOverridePrintf(&crash_dump_printf);
    (void)FPFwCDDumpManagerSetPreDumpCallback(&type_context->crash_dump_ctx, &preDumpCallbackOverride, type_context);
    (void)FPFwCDDumpManagerSetPostDumpCallback(&type_context->crash_dump_ctx, &postDumpCallbackOverride, type_context);
    (void)FPFwCDDumpManagerOverrideGetCurTime(&type_context->crash_dump_ctx, &getCurTimeDefault);
}

/**
 * @brief Captures built-in Cortex M7 registers
 *
 * @param type_context crash dump type based context
 * @param accel_type accel type
 * This will be a dummy register capture and will only capture the level-1 register value
 * r0 - level-1 register value
 * r1 - actual interrupt seen on scp
 */
void crash_dump_accel_core_register_reg(crash_dump_accel_ctx_t* type_context, ACCEL_ID accel_type)
{
    if (type_context)
    {
        CdRegisterRegisterSet(&type_context->crash_dump_ctx,
                              &g_accel_core_crash_context[accel_type],
                              CORE_BUILTIN_REG_INDEX,
                              sizeof(g_accel_core_crash_context[accel_type]) / (sizeof(uint32_t)),
                              FPFW_CD_DUMP_PRIORITY_CRITICAL);
    }
}

/*------------- Public Functions ----------------*/

void crash_dump_generate_default_accel_cd(ACCEL_ID accel_type)
{
    FPFwCdBugCheckInfo bug_check_info = {0};
    uint32_t prid = CRASH_DUMP_PROCESSOR_ID(idsw_get_die_id(),
                                            (accel_type == ACCEL_ID_SDM) ? CRASH_DUMP_CORE_SDM : CRASH_DUMP_CORE_CDED);
    uint32_t accel_irq_isr_ptr = 0;
    uint32_t accel_irq_mask_ptr = 0;
    uint32_t* accel_irq_bh_val = 0;
    uint32_t accl_irq_isr_val = 0;

    s_crash_dump_accel_ctx[accel_type].crash_dump_ctx.prid = prid;
    bug_check_info.coreIndex = prid;

    /**
     * Core context will be modified to show the following values
     * 1. r0 will have the dummy bug check code 0xdeadbeef
     * 2. r1 will be accel isr value captured in virt irq
     * 3. r2 will be isr reg value from the bottom half
     */

    accel_intr_virt_irq_cd_reg(accel_type, &accel_irq_isr_ptr, &accel_irq_mask_ptr);
    accel_irq_bh_val = (uint32_t*)accel_intr_get_bh_irq_val_addr(accel_type);
    accl_irq_isr_val = (uint32_t)(*((uint32_t*)(uintptr_t)accel_irq_isr_ptr));

    // The error code will indicate this is a default CD with very limited info
    bug_check_info.data.Code = KNG_CD_ACCEL_DEFAULT_CD; // Error code indicates default CD generate

    g_accel_core_crash_context[accel_type].r0 = KNG_CD_ACCEL_DEFAULT_CD;
    g_accel_core_crash_context[accel_type].r1 = accl_irq_isr_val;
    g_accel_core_crash_context[accel_type].r2 = (uint32_t)(*accel_irq_bh_val); // NOLINT

    FPFwCDCrashDumpHandler(&s_crash_dump_accel_ctx[accel_type].crash_dump_ctx,
                           &g_accel_core_crash_context[accel_type],
                           &bug_check_info);
}

void crash_dump_default_accel_cd_init(ACCEL_ID accel_type)
{
    crash_dump_accel_ctx_t* type_context = &s_crash_dump_accel_ctx[accel_type];

    // Initialize the accel crash dump context based on the accel type
    if (accel_type == ACCEL_ID_SDM)
    {
        type_context->mem_pool_addr = (idsw_get_die_id() == DIE_0) ? CRASH_DUMP_FULL_SDM0_ADDR : CRASH_DUMP_FULL_SDM1_ADDR;
        type_context->mem_pool_size = CRASH_DUMP_FULL_SIZE_PER_CORE;
    }
    else
    {
        type_context->mem_pool_addr = (idsw_get_die_id() == DIE_0) ? CRASH_DUMP_FULL_CDED0_ADDR : CRASH_DUMP_FULL_CDED1_ADDR;
        type_context->mem_pool_size = CRASH_DUMP_FULL_SIZE_PER_CORE;
    }

    FPFW_DBGPRINT_INFO("Creating default crash dump for accel %d at addr 0x%llx with size 0x%llx\r\n",
                       accel_type,
                       (long long unsigned int)type_context->mem_pool_addr,
                       (long long unsigned int)type_context->mem_pool_size);

    init_dump_desc(&s_crash_dump_accel_ctx[accel_type]);
    init_mem_pool(&s_crash_dump_accel_ctx[accel_type]);
    init_dump_file(&s_crash_dump_accel_ctx[accel_type]);
    init_dump_manager(&s_crash_dump_accel_ctx[accel_type]);

    crash_dump_accel_core_register_reg(type_context, accel_type);
}

/**
 * @brief Copies accel device crashdump from accel DTCM to DDR
 *
 */
void crash_dump_copy_accel_cd_file(void* ctx)
{
    crash_dump_context_t* cd_ctx = crash_dump_context();
    ACCEL_ID accel_type = (ACCEL_ID)ctx;

    if (cd_ctx == NULL || accel_type >= NUM_VALID_ACCEL_ID)
    {
        FPFW_DBGPRINT_ERROR("ACC%d_CD_CPY_INVALID_ARGS\r\n", (int)accel_type);
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_ADDRESS, __LINE__);
        return;
    }

    /* Copy accel crashdump file from accel DTCM to DDR */
    copy_cd_file_dtcm_to_ddr(cd_ctx, accel_type);
}

uint32_t crash_dump_transfer_accel_cd_to_BMC(ACCEL_ID accel_type)
{
    FPFW_DBGPRINT_INFO("ACC%d_CD_BMC_XFER_START\r\n", (int)accel_type);
    crash_dump_copy_accel_cd_file((void*)accel_type);

    return crash_dump_request_transfer_dump();
}

bool crash_dump_is_accel_cd_complete(ACCEL_ID accel_type)
{
    crash_dump_context_t* cd_ctx = crash_dump_context();
    uint32_t* cd_magic_nr;

    if (cd_ctx == NULL)
    {
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_ADDRESS, __LINE__);
        return false;
    }

    cd_magic_nr = (void*)cd_ctx->accel_cd_ctx[accel_type].cd_magic_nr_offset;
    if ((uint32_t)cd_magic_nr + sizeof(*cd_magic_nr) > SDM_EXT_CFG_EMCPU_TCM_DTCM_SIZE)
    {
        // Invalid magic number address.
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_INVALID_SIZE, __LINE__);
        return false;
    }

    // Compute the ATU mapped address of the magic number
    uint32_t accel_dtcm_addr = atu_svc_accel_atu_addr(accel_type) + SDM_EXT_CFG_EMCPU_TCM_DTCM_ADDRESS;
    cd_magic_nr = (void*)cd_magic_nr + accel_dtcm_addr;

    if (MMIO_READ32(cd_magic_nr) != CD_MAGIC_NUMBER)
    {
        // SDM/CDED busy
        CRASH_DUMP_ET_ERROR_PARAM(CRASH_DUMP_ET_TYPE_ACCEL_MAGIC_NUMBER_UNMATCHED, __LINE__);
        return false;
    }

    return true;
}

uint64_t get_timestamp_from_cd(DUMP_CHUNK_CRASH_INFORMATION* p_dump_crash_info)
{
    return ((uint64_t)p_dump_crash_info->Tcon[1] << 32) + (uint64_t)p_dump_crash_info->Tcon[0];
}

void update_timestamp_in_cd(DUMP_CHUNK_CRASH_INFORMATION* p_dump_crash_info, uint64_t utc_crash_time_ms)
{
    p_dump_crash_info->Tcon[0] = (uint32_t)((utc_crash_time_ms) & 0xFFFFFFFFUL);       // NOLINT
    p_dump_crash_info->Tcon[1] = (uint32_t)((utc_crash_time_ms >> 32) & 0xFFFFFFFFUL); // NOLINT
}

void crash_dump_register_accel_cd()
{
    uint32_t irq_th_val = 0;
    uint32_t irq_th_mask = 0;
    uint32_t irq_bh_val = 0;

    if (idsw_get_cpu_type() == CPU_MCP)
    {
        // This API is not needed for MCP as only SCP co-ordinates accel CD-flow
        return;
    }

    for (ACCEL_ID id = 0; id < NUM_VALID_ACCEL_ID; id++)
    {
        accel_intr_virt_irq_cd_reg(id, &irq_th_val, &irq_th_mask);
        irq_bh_val = (uint32_t)accel_intr_get_bh_irq_val_addr(id);

        crash_dump_register_address32((void*)irq_th_val, sizeof(uint32_t), FPFW_CD_DUMP_PRIORITY_CRITICAL);
        crash_dump_register_address32((void*)irq_th_mask, sizeof(uint32_t), FPFW_CD_DUMP_PRIORITY_CRITICAL);
        crash_dump_register_address32((void*)irq_bh_val, sizeof(uint32_t), FPFW_CD_DUMP_PRIORITY_CRITICAL);
    }
}
