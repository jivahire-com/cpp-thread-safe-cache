/*
 * Copyright (c) Microsoft Corporation.
 *
 * Description:
 *     Implementation of power logging
 */

/*------------- Includes -----------------*/
#include "power_events.h"
#include "power_i.h"

#include <FpFwUtils.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <corebits.h>
#include <crash_dump.h>
#include <ddrss_reserved_regions.h>
#include <idsw.h> // SW platform id
#include <idsw_kng.h>
#include <power_dfwk.h>      // for (anonymous), ppower_service_cli_re...
#include <power_init.h>      // for power_init, power_interface_init
#include <power_log.h>       // for power_init, power_interface_init
#include <silibs_platform.h> // for MMIO_WRITE32, MMIO_READ32, MMIO_WRITE16, MMIO_WRITE8
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

// macro for inserting a power log into event trace
static_assert(sizeof(power_log_entry_t) <= (sizeof(uint64_t) * POWER_ET_LOG_TRACE_UINT64_COUNT), "Log entry size does not fit into POWER_ET_LOG_TRACE_UINT64_COUNT uint64_ts needed for trace - increase trace parameters");
#define POWER_ET_LOG_TRACE(entry)                                                                                            \
    do                                                                                                                       \
    {                                                                                                                        \
        /* parameter should be a pointer to a power_log_entry_t */                                                           \
        static_assert(sizeof(power_log_entry_t) == sizeof(*entry), "Check parameter type");                                  \
        EventWritePowerLogTrace(((uint64_t*)entry)[0], ((uint64_t*)entry)[1], ((uint64_t*)entry)[2], ((uint64_t*)entry)[3]); \
    } while (0)

#define MAX_LOG_CHARS 256
static char string_buffer[MAX_LOG_CHARS];
static bool should_use_ddr = false;
static power_log_data_t log = {};

/*------------- Functions ----------------*/

void mmio_ap_mem_cpy(uint64_t globalAddr, uintptr_t localAddr, uint64_t numBytes)
{
    uint64_t bytesCopied = 0;

    atu_map_entry_t log_mem_atu_map_struct;
    atu_entry_attr_t log_test_atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT};

    uint64_t start_addr = 0;
    uint64_t end_addr = 0;

    // set up the mapping for the DDR log
    start_addr = globalAddr;
    if (idsw_get_die_id() == DIE_0)
    {
        end_addr = (POWER_LOG_DDR_BASE_DIE0 + POWER_LOG_DDR_SIZE_DIE0);
        BUG_ASSERT((start_addr + numBytes < end_addr) && (start_addr >= POWER_LOG_DDR_BASE_DIE0));
    }
    else
    {
        end_addr = (POWER_LOG_DDR_BASE_DIE1 + POWER_LOG_DDR_SIZE_DIE1);
        BUG_ASSERT((start_addr + numBytes < end_addr) && (start_addr >= POWER_LOG_DDR_BASE_DIE1));
    }
    log_mem_atu_map_struct.ap_base_address = start_addr;
    log_mem_atu_map_struct.mscp_start_address = 0;
    log_mem_atu_map_struct.mscp_end_address = end_addr - 1;
    log_mem_atu_map_struct.attribute.as_uint32 = log_test_atu_root_attr.as_uint32;

    if (!atu_map(ATU_ID_MSCP, &log_mem_atu_map_struct))
    {
        printf("  ATU mapping failed\n");
    }
    else
    {
        printf("  ATU mapping succeeded\n");
    }

    while (bytesCopied < numBytes)
    {
        uint64_t bytesLeft = numBytes - bytesCopied;

        if (bytesLeft >= sizeof(uint64_t))
        {
            MMIO_WRITE64(log_mem_atu_map_struct.mscp_start_address + bytesCopied,
                         *(uint64_t*)((uint8_t*)localAddr + bytesCopied));
            bytesCopied += sizeof(uint64_t);
        }
        else if (bytesLeft >= sizeof(uint32_t))
        {
            MMIO_WRITE32(log_mem_atu_map_struct.mscp_start_address + bytesCopied,
                         *(uint32_t*)((uint8_t*)localAddr + bytesCopied));
            bytesCopied += sizeof(uint32_t);
        }
        else if (bytesLeft >= sizeof(uint16_t))
        {
            MMIO_WRITE16(log_mem_atu_map_struct.mscp_start_address + bytesCopied,
                         *(uint16_t*)((uint8_t*)localAddr + bytesCopied));
            bytesCopied += sizeof(uint16_t);
        }
        else
        {
            MMIO_WRITE8(log_mem_atu_map_struct.mscp_start_address + bytesCopied,
                        *(uint8_t*)((uint8_t*)localAddr + bytesCopied));
            bytesCopied += sizeof(uint8_t);
        }
    }
    atu_unmap(ATU_ID_MSCP, &log_mem_atu_map_struct);
}

power_log_data_t* get_instance()
{
    power_log_data_t* log_data_instance = &log;
    return log_data_instance;
}

char* power_log_string(uint8_t type, power_log_payload_t* payload)
{
#define POWER_LOG_STRING_CASES(name, id, payload_type, has_core_field, fmt_str, ...) \
    case name:                                                                       \
        snprintf(string_buffer, sizeof(string_buffer), fmt_str __VA_ARGS__);         \
        break;

    switch (type)
    {
        POWER_LOG_GENERATOR(POWER_LOG_STRING_CASES);
    }
    return string_buffer;
}

/* function to return true if payload for type has cores member */
bool power_log_has_cores(uint8_t type)
{
#define POWER_LOG_HAS_CORES_CASES(name, id, payload_type, has_core_field, fmt_str, ...) \
    case name:                                                                          \
        return has_core_field;                                                          \
        break;

    switch (type)
    {
        POWER_LOG_GENERATOR(POWER_LOG_HAS_CORES_CASES);
    }
    return false;
}

void power_log_init()
{
    // power_log_data_t *log = &power_context.log;
    //  allocate memory for log entries
    log.entries = malloc(POWER_LOG_LOCAL_SIZE);
    if (log.entries == NULL)
    {
        free(log.entries);
    }
    BUG_ASSERT(log.entries != NULL);
    // setup entry count based on size of allocation
    log.max_entries = POWER_LOG_LOCAL_SIZE / sizeof(power_log_entry_t);
    // invalidate first entry
    log.entries[0].type = POWER_LOG_INVALID_TYPE;
    // initialize log mask
    log.mask = UINT32_MAX & ~DIST_MIN_PLIMIT_UPD_MASK;
    // register the power log with crash dump
    // need crashdump
    crash_dump_register_address32((void*)&log.entries, POWER_LOG_LOCAL_SIZE, FPFW_CD_DUMP_PRIORITY_NORMAL);
}

/* sets the use DDR flag and resets the DDR content (on enable) */
void power_log_use_ddr(bool use_ddr)
{
    should_use_ddr = use_ddr;
    if (use_ddr)
    {
        power_log_entry_t invalid_entry = {.type = POWER_LOG_INVALID_TYPE};
        // reset DDR log
        if (idsw_get_die_id() == DIE_0)
        {
            log.max_ddr_entries = (POWER_LOG_DDR_SIZE_DIE0) / sizeof(power_log_entry_t);
        }
        else
        {
            log.max_ddr_entries = (POWER_LOG_DDR_SIZE_DIE1) / sizeof(power_log_entry_t);
        }
        log.last_ddr_entry = 0;
        log.oldest_ddr_entry = 0;
        // create an initial invalid entry
        if (idsw_get_die_id() == DIE_0)
        {
            mmio_ap_mem_cpy(POWER_LOG_DDR_ENTRY_ADDR(POWER_LOG_DDR_BASE_DIE0, 0),
                            (uintptr_t)&invalid_entry,
                            sizeof(power_log_entry_t));
        }
        else
        {
            mmio_ap_mem_cpy(POWER_LOG_DDR_ENTRY_ADDR(POWER_LOG_DDR_BASE_DIE1, 0),
                            (uintptr_t)&invalid_entry,
                            sizeof(power_log_entry_t));
        }
    }
}

/* for a given entry index, returns the next, wrapping at end of log */
static inline unsigned next_entry(unsigned entry)
{
    entry += 1;
    entry %= log.max_entries;
    return entry;
}

/* for a given DDR entry index, returns the next, wrapping at end of log */
static inline unsigned next_ddr_entry(unsigned entry)
{
    entry += 1;
    entry %= log.max_ddr_entries;
    return entry;
}

static void add_ddr_entry(power_log_entry_t* log_entry)
{
    ASSERT(log_entry != NULL);

    // increment to next entry
    log.last_ddr_entry = next_ddr_entry(log.last_ddr_entry);
    if (log.last_ddr_entry == log.oldest_ddr_entry)
    {
        log.oldest_ddr_entry = next_ddr_entry(log.oldest_ddr_entry);
    }
    // copy entry to DDR
    if (idsw_get_die_id() == DIE_0)
    {
        mmio_ap_mem_cpy(POWER_LOG_DDR_ENTRY_ADDR(POWER_LOG_DDR_BASE_DIE0, log.last_ddr_entry),
                        (uintptr_t)log_entry,
                        sizeof(power_log_entry_t));
    }
    else
    {
        mmio_ap_mem_cpy(POWER_LOG_DDR_ENTRY_ADDR(POWER_LOG_DDR_BASE_DIE1, log.last_ddr_entry),
                        (uintptr_t)log_entry,
                        sizeof(power_log_entry_t));
    }
}

void power_log_update_timestamp(uint64_t timestamp)
{
    // power_log_data_t *log  = &power_context.log;
    log.last_timestamp = timestamp;
    unsigned previous_last = log.last_timestamp_entry;
    // track the entry when the timestamp changed; it'll be the next updated one
    log.last_timestamp_entry = next_entry(log.last_entry);
    // clear type of next entry to be invalid
    log.entries[log.last_timestamp_entry].type = POWER_LOG_INVALID_TYPE;

    // iterate over entries since last timestamp change
    while (previous_last != log.last_timestamp_entry)
    {
        // copy to DDR
        if (should_use_ddr)
        {
            add_ddr_entry(&log.entries[previous_last]);
        }
        // send to event trace
        POWER_ET_LOG_TRACE(&log.entries[previous_last]);
        previous_last = next_entry(previous_last);
    }
}

/* this is the main log function; takes a list of cores the log applies to, a type, and the type's payload */
void power_log_cores(const corebits_t* cores, uint8_t type, power_log_payload_t* payload)
{
    // power_log_data_t *log               = &power_context.log;
    bool found = false;
    unsigned entry_idx = log.last_timestamp_entry;
    const unsigned last_iteration_entry = next_entry(log.last_entry);
    const bool has_cores = power_log_has_cores(type);

    // skip log if not in mask
    if (((1 << type) & log.mask) == 0)
    {
        return;
    }

    // if a type has a cores field, then attempt to only update the field if a previous log already has the same timestamp and payload
    if (has_cores)
    {
        for (; entry_idx != last_iteration_entry; entry_idx = next_entry(entry_idx))
        {
            if (log.entries[entry_idx].type == POWER_LOG_INVALID_TYPE)
            {
                break;
            }
            if ((log.entries[entry_idx].type == type) && (memcmp(CORES_OFFSET(payload),
                                                                 CORES_OFFSET(&log.entries[entry_idx].payload),
                                                                 sizeof(power_log_payload_t) - sizeof(corebits_t)) == 0))
            {
                // found an identical message at this timestamp
                found = true;
                break;
            }
        }
    }
    else
    {
        // if no cores field, we'll always create a new entry
        entry_idx = last_iteration_entry;
    }
    if (found)
    {
        // update an old entry
        corebits_or(&(log.entries[entry_idx].payload.cores), cores);
    }
    else
    {
        // create a new entry
        if (entry_idx == log.oldest_entry)
        {
            log.oldest_entry = next_entry(log.oldest_entry);
        }
        log.last_entry = entry_idx;
        log.entries[entry_idx].timestamp = log.last_timestamp;
        log.entries[entry_idx].type = type;
        log.entries[entry_idx].payload = *payload;
        if (has_cores)
        {
            log.entries[entry_idx].payload.cores = *cores;
        }
    }
}

void power_log_core(unsigned int core, uint8_t type, power_log_payload_t* payload)
{
    corebits_t new_pending = {0};
    corebits_set_bit(&new_pending, core);
    power_log_cores(&new_pending, type, payload);
}

void power_log_cores_ts(uint64_t timestamp, const corebits_t* cores, uint8_t type, power_log_payload_t* payload)
{
    power_log_update_timestamp(timestamp);
    power_log_cores(cores, type, payload);
}
