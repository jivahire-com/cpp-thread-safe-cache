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
static bool ddr_logging_enabled = false;
static power_log_data_t log = {};
static power_log_entry_t s_log_entries[POWER_LOG_LOCAL_SIZE / sizeof(power_log_entry_t)];

/*------------- Functions ----------------*/
int power_log_ddr_map_unmap(bool map, KNG_DIE_ID die_id)
{
    int status = SILIBS_SUCCESS;
    atu_map_entry_t log_mem_atu_map_struct;
    atu_entry_attr_t log_test_atu_root_attr = {ATU_BUS_ATTR_ROOT};
    uint64_t start_addr = 0;
    uint32_t size = 0;

    // set up the mapping for the DDR log
    if (die_id == DIE_0)
    {
        start_addr = POWER_LOG_DDR_ENTRY_ADDR(POWER_LOG_DDR_BASE_DIE0, 0);
        size = POWER_LOG_DDR_SIZE_DIE0;
    }
    else
    {
        start_addr = POWER_LOG_DDR_ENTRY_ADDR(POWER_LOG_DDR_BASE_DIE1, 0);
        size = POWER_LOG_DDR_SIZE_DIE1;
    }

    //! In atu_lib implementation, if mscp_start_addr is 0, a free mscp address region will be assigned for
    //! mapping in that case, the size of the region is given by mscp_end_addr - 1
    log_mem_atu_map_struct.ap_base_address = ALIGN_DOWN(start_addr, ATU_PAGE_SIZE);
    log_mem_atu_map_struct.mscp_start_address = 0;
    log_mem_atu_map_struct.mscp_end_address = ALIGN_UP(size, ATU_PAGE_SIZE) - 1;
    log_mem_atu_map_struct.attribute.as_uint32 = log_test_atu_root_attr.as_uint32;

    //! Check if the mapping already exists
    if (atu_find_map(ATU_ID_MSCP, &log_mem_atu_map_struct) == SILIBS_SUCCESS)
    {
        POWER_LOG_INFO("Found existing mapping for address 0x%" PRIx64 " at 0x%" PRIx32 "\n",
                       log_mem_atu_map_struct.ap_base_address,
                       log_mem_atu_map_struct.mscp_start_address);

        //! If the mapping is not needed, unmap it
        if (!map)
        {
            status = atu_unmap(ATU_ID_MSCP, &log_mem_atu_map_struct);
            if (status != SILIBS_SUCCESS)
            {
                POWER_LOG_ERR("Pwr Log DDR failed due to ATU unmapping failure, Stat:%d\n", status);
                return status;
            }
            //! Unmap was successful, reset the cached mscp start address
            log.mapped_mscp_addr = 0UL;
        }
    }
    else //! No mapping found, map if requested
    {
        POWER_LOG_INFO("No existing mapping for address 0x%" PRIx64 "\n", log_mem_atu_map_struct.ap_base_address);
        if (map)
        {
            status = atu_map(ATU_ID_MSCP, &log_mem_atu_map_struct);
            if (status != SILIBS_SUCCESS)
            {
                POWER_LOG_ERR("Pwr Log DDR failed due to ATU mapping failure, Stat:%d\n", status);
            }
            else
            {
                //! Map was successful, save the mscp start address
                log.mapped_mscp_addr = log_mem_atu_map_struct.mscp_start_address;
                POWER_LOG_INFO("Pwr Log DDR mapping successful, AP addr:0x%" PRIx64 ", MSCP addr:0x%" PRIx32
                               ", Size: 0x%" PRIx32 "\n",
                               log_mem_atu_map_struct.ap_base_address,
                               log.mapped_mscp_addr,
                               ALIGN_UP(size, ATU_PAGE_SIZE));
            }
        }
    }
    return status;
}

void mmio_ap_mem_cpy(uintptr_t mscpMappedAddr, uintptr_t localAddr, uint32_t numBytes)
{
    uint32_t bytesCopied = 0;
    uint32_t MscpPwrLogBaseAddr = (uint32_t)mscpMappedAddr;

    BUG_ASSERT(mscpMappedAddr != 0);
    BUG_ASSERT(localAddr != 0);
    BUG_ASSERT(numBytes != 0);
    BUG_ASSERT(numBytes <= POWER_LOG_DDR_SIZE_DIE0);

    while (bytesCopied < numBytes)
    {
        uint32_t bytesLeft = numBytes - bytesCopied;

        if (bytesLeft >= sizeof(uint64_t))
        {
            MMIO_WRITE64(MscpPwrLogBaseAddr + bytesCopied, *(uint64_t*)((uint8_t*)localAddr + bytesCopied));
            bytesCopied += sizeof(uint64_t);
        }
        else if (bytesLeft >= sizeof(uint32_t))
        {
            MMIO_WRITE32(MscpPwrLogBaseAddr + bytesCopied, *(uint32_t*)((uint8_t*)localAddr + bytesCopied));
            bytesCopied += sizeof(uint32_t);
        }
        else if (bytesLeft >= sizeof(uint16_t))
        {
            MMIO_WRITE16(MscpPwrLogBaseAddr + bytesCopied, *(uint16_t*)((uint8_t*)localAddr + bytesCopied));
            bytesCopied += sizeof(uint16_t);
        }
        else
        {
            MMIO_WRITE8(MscpPwrLogBaseAddr + bytesCopied, *(uint8_t*)((uint8_t*)localAddr + bytesCopied));
            bytesCopied += sizeof(uint8_t);
        }
    }
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
    if (log.initialized)
    {
        return;
    }
    // power_log_data_t *log = &power_context.log;
    // use static array for log entries
    log.entries = s_log_entries;

    // setup entry count based on size of allocation
    log.max_entries = POWER_LOG_LOCAL_SIZE / sizeof(power_log_entry_t);
    // invalidate first entry
    log.entries[0].type = POWER_LOG_INVALID_TYPE;
    // initialize log mask
    log.mask = UINT32_MAX & ~DIST_MIN_PLIMIT_UPD_MASK;
    // register the power log with crash dump
    // need crashdump
    crash_dump_register_address32((void*)&log.entries, POWER_LOG_LOCAL_SIZE, FPFW_CD_DUMP_PRIORITY_NORMAL);
    POWER_LOG_INFO("Pwr Log Init, Size[%u] MaxEntries[%u] Mask[0x%" PRIx32 "]\n", POWER_LOG_LOCAL_SIZE, log.max_entries, log.mask);
    log.initialized = true;
}

/* sets the use DDR flag and resets the DDR content (on enable) */
int power_log_use_ddr(bool use_ddr)
{
    KNG_DIE_ID die_id = idsw_get_die_id();
    if (log.initialized == false)
    {
        POWER_LOG_ERR("Power log not initialized\n");
        return SILIBS_E_INIT;
    }

    //! Map/unmap as per user input, takes care of case where DDR is already mapped
    BUG_ASSERT(power_log_ddr_map_unmap(use_ddr, die_id) == SILIBS_SUCCESS);

    //! compare previous ddr enable status, if not enabled already, reset ddr entries, add an invalid entry to DDR
    if (!ddr_logging_enabled && (use_ddr == true))
    {
        power_log_entry_t invalid_entry = {.type = POWER_LOG_INVALID_TYPE};
        // reset DDR log & create an initial invalid entry
        log.last_ddr_entry = 0;
        log.oldest_ddr_entry = 0;
        log.max_ddr_entries = (POWER_LOG_DDR_SIZE_DIE0) / sizeof(power_log_entry_t);
        mmio_ap_mem_cpy((uintptr_t)POWER_LOG_DDR_MAPPED_MSCP_ENTRY_ADDR(log.mapped_mscp_addr, 0),
                        (uintptr_t)&invalid_entry,
                        sizeof(power_log_entry_t));
    }

    //! Update the ddr log status
    ddr_logging_enabled = use_ddr;
    POWER_LOG_INFO("DDR Power Logging %s for Die ID[%d]\n", ddr_logging_enabled ? "Enabled" : "Disabled", die_id);
    return SILIBS_SUCCESS;
}

/* for a given entry index, returns the next, wrapping at end of log */
static inline unsigned next_entry(unsigned entry)
{
    if (log.max_entries == 0)
    {
        POWER_LOG_ERR("Power log not initialized or max entries is 0\n");
        return 0;
    }
    entry += 1;
    entry %= log.max_entries;
    return entry;
}

/* for a given DDR entry index, returns the next, wrapping at end of log */
static inline unsigned next_ddr_entry(unsigned entry)
{
    if (log.max_ddr_entries == 0)
    {
        POWER_LOG_ERR("Power log DDR not initialized or max_ddr_entries is 0\n");
        return 0;
    }

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
    mmio_ap_mem_cpy((uintptr_t)POWER_LOG_DDR_MAPPED_MSCP_ENTRY_ADDR(log.mapped_mscp_addr, log.last_ddr_entry),
                    (uintptr_t)log_entry,
                    sizeof(power_log_entry_t));
}

void power_log_update_timestamp(uint64_t timestamp)
{
    if (!log.initialized)
    {
        POWER_LOG_ERR("Power log not initialized\n");
        return;
    }

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
        if (ddr_logging_enabled)
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
    if (!log.initialized)
    {
        POWER_LOG_ERR("Power log not initialized\n");
        return;
    }

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
    if (!log.initialized)
    {
        POWER_LOG_ERR("Power log not initialized\n");
        return;
    }

    corebits_t new_pending = {0};
    corebits_set_bit(&new_pending, core);
    power_log_cores(&new_pending, type, payload);
}

void power_log_cores_ts(uint64_t timestamp, const corebits_t* cores, uint8_t type, power_log_payload_t* payload)
{
    if (!log.initialized)
    {
        POWER_LOG_ERR("Power log not initialized\n");
        return;
    }

    power_log_update_timestamp(timestamp);
    power_log_cores(cores, type, payload);
}
