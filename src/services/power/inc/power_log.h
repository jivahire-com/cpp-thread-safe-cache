//
// Copyright (c) Microsoft Corporation.
//

/**
 * @file power_log.h
 * Private header for soc power module logging
 */

#pragma once

/*----------- Nested includes ------------*/
#include "corebits.h"

#include <ddrss_reserved_regions.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/
#define POWER_LOG_INVALID_TYPE UINT8_MAX
#define VM_THROT_COUNT         8

/* DDR memory to use for log to DDR - only enabled by CLI, not for general use */
#define FW_UPDATE_USED_DIE0                                 (1 * 1024 * 1024)  // 2MB used in DDR (outside of actual update)
#define FW_UPDATE_USED_DIE1                                 (1 * 1024 * 1024)  // 2MB used in DDR (outside of actual update)
#define POWER_LOG_DDR_BASE_DIE0                             (POWER_LOG_RESERVATION_BASE)
#define POWER_LOG_DDR_BASE_DIE1                        (POWER_LOG_RESERVATION_BASE+ FW_UPDATE_USED_DIE0)
#define POWER_LOG_DDR_SIZE_DIE0                            (FW_UPDATE_USED_DIE0)
#define POWER_LOG_DDR_SIZE_DIE1                             (FW_UPDATE_USED_DIE1)
#define POWER_LOG_DDR_ENTRY_ADDR(base_addr, entry_idx) (((uint64_t)(base_addr)) + (sizeof(power_log_entry_t) * entry_idx))

#define POWER_LOG_LOCAL_SIZE (2048)  // local log size; always present

// below are the log types, their associated payload types, and a string to print for the given entry
// clang-format off
#define POWER_LOG_GENERATOR(log) \
    log(DIST_MIN_PLIMIT_UPD, 0, power_log_empty_t,    true, "Min plimit pending updates", /* no data */) \
    log(DIST_AVAIL,          1, power_log_dist_avl_t, false, "Total pwr: %1.3fW Vcpu %1.3fW  Cap: %1.3fW Vcpu %1.3fW", /* intentionally left blank */, payload->members.DIST_AVAIL.soc_pwr, payload->members.DIST_AVAIL.vcpu_pwr, payload->members.DIST_AVAIL.soc_cap, payload->members.DIST_AVAIL.vcpu_cap) \
    log(PID,                 2, power_log_pid_t,      false, "PID calculate: error %1.3f p %1.3f i %1.3f d %1.3f avail %" PRId32 " scaled %" PRId32 "", /* intentionally left blank */, payload->members.PID.error, payload->members.PID.p, payload->members.PID.i, payload->members.PID.d, payload->members.PID.available, payload->members.PID.scaled) \
    log(PLIMIT,              3, power_log_plimit_t,   true,  "plimit selected %d rack cap %d", /**/, payload->members.PLIMIT.plimit, (uint8_t)payload->members.PLIMIT.rack_cap) \
    log(TP_SELECTION,        4, power_log_thrpri_t,   false, "Per priority plimit selections %d %d %d %d %d %d %d %d", /**/, payload->members.TP_SELECTION.selection[0], payload->members.TP_SELECTION.selection[1], payload->members.TP_SELECTION.selection[2], payload->members.TP_SELECTION.selection[3], \
                                                                                                                             payload->members.TP_SELECTION.selection[4], payload->members.TP_SELECTION.selection[5], payload->members.TP_SELECTION.selection[6], payload->members.TP_SELECTION.selection[7]) \
    /* keep the below for test code */ \
    log(TEST_ENTRY_NO_CORES, 30, power_log_empty_t, false, "TE1 - %s", /**/, payload->all) \
    log(TEST_ENTRY_CORES, 31, power_log_empty_t, true, "TE2", /**/) \
// clang-format on

// simplifies providing type+payload to log function
#define POWER_LOG_DATA(name, ...) \
    name, &(power_log_payload_t) \
    { \
        .name = __VA_ARGS__ \
    }

// pointer to first byte past cores structure within payload
#define CORES_OFFSET(payload) ((void *)((uintptr_t)payload + sizeof(corebits_t)))

/*-------------- Typedefs ----------------*/
// payload structs
#pragma pack(push, 1)
typedef void *power_log_empty_t;
typedef struct {
    float soc_pwr;
    float soc_cap;
    float vcpu_pwr;
    float vcpu_cap;
} power_log_dist_avl_t;

typedef struct {
    float error;
    float p;
    float i;
    float d; 
    uint32_t available;
    uint32_t scaled;
} power_log_pid_t;

typedef struct {
    corebits_t cores;
    uint8_t plimit;
    bool rack_cap;
} power_log_plimit_t;

typedef struct {
    uint8_t selection[VM_THROT_COUNT];
} power_log_thrpri_t;
#pragma pack(pop)

// generates payload entries
#define UNION_MEMBERS(name, id, payload_type, has_core_field, fmt_str, ...) payload_type name;
typedef union {
    POWER_LOG_GENERATOR(UNION_MEMBERS)
} PowerLogUnion;

#define MAX_PAYLOAD_SIZE sizeof(PowerLogUnion)
typedef union {
    char all[MAX_PAYLOAD_SIZE];  // read that ={0} only clears to size of first entry, so ensuring first entry is the max size of any entry
    PowerLogUnion members;
    corebits_t cores;
} power_log_payload_t;

// struct for an entry in the log
typedef struct _power_log_entry {
    uint64_t timestamp : 56;
    uint64_t type : 8;
    power_log_payload_t payload;
} power_log_entry_t;

// log state structure
typedef struct _power_log_data {
    power_log_entry_t *entries;
    unsigned max_entries;
    unsigned last_entry;
    unsigned oldest_entry;
    unsigned last_timestamp_entry;
    uint64_t last_timestamp;
    unsigned max_ddr_entries;
    unsigned last_ddr_entry;
    unsigned oldest_ddr_entry;
    uint32_t mask;
} power_log_data_t;

// generates log type enums
#define POWER_LOG_ENUMS(name, id, payload_type, has_core_field, fmt_str, ...) name = id,
typedef enum
{
    POWER_LOG_GENERATOR(POWER_LOG_ENUMS)
} power_log_types_t;

/*-- Declarations (Statics and globals) --*/
static const corebits_t ALLCORES = {{0xFFFFFFFF, 0xFFFFFFFF, 0x0F}};

// mask bits
#define MASK_BITS(name, id, payload_type, has_core_field, fmt_str, ...) static const uint32_t name##_MASK=(1 << id);
POWER_LOG_GENERATOR(MASK_BITS)


/*--------- Function Prototypes ----------*/

void mmio_ap_mem_cpy(uint64_t globalAddr, uintptr_t localAddr, uint64_t numBytes);
power_log_data_t *get_instance();
void power_log_init();
void power_log_use_ddr(bool use_ddr);
void power_log_update_timestamp(uint64_t timestamp);
void power_log_core(unsigned int core, uint8_t type, power_log_payload_t *payload);
void power_log_cores(const corebits_t *cores, uint8_t type, power_log_payload_t *payload);
void power_log_cores_ts(uint64_t timestamp, const corebits_t *cores, uint8_t type, power_log_payload_t *payload);
char *power_log_string(uint8_t type, power_log_payload_t *payload);
bool power_log_has_cores(uint8_t type);
