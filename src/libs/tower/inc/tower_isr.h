//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file tower_isr.h
 *  ISR interface and error injection utilities for NI-Tower
 */

#pragma once

/*--------------- Includes ---------------*/

#include <health_monitor.h>
#include <kng_soc_constants.h>
#include <silibs_platform.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/

typedef enum
{
    TOWER_EINJ_TARGET_BY_NODE = 0,
    TOWER_EINJ_TARGET_BY_INDEX = 1,
    TOWER_EINJ_TARGET_RAW = 2,
} TOWER_EINJ_OPERATION;

#pragma pack(push, 1)

/* This structure maps to ras_einj_info_t.param_type.error_parameters[0] */
typedef union _tower_error_inj_op_type_t
{
    struct
    {
        uint32_t op;   /* TOWER_EINJ_OPERATION */
        uint32_t type; /* TOWER_ERR_PROTECTION_TYPE, Not used by TOWER_EINJ_TARGET_RAW */
    };
    uint64_t as_uint64;
} tower_error_inj_op_type_t;

static_assert(sizeof(tower_error_inj_op_type_t) == sizeof(uint64_t), "'tower_error_inj_op_type_t' is not the expected size!");

/* This structure maps to ras_einj_info_t.param_type.error_parameters[1] */
typedef union
{
    struct
    {
        uint64_t node_type : 16; /* TOWER_NODE_TYPE */
        uint64_t node_id : 16;
        uint64_t clock_domain_id : 10;
        uint64_t power_domain_id : 10;
        uint64_t voltage_domain_id : 10;
        uint64_t reserved : 2;
    } by_node;
    struct
    {
        uint32_t index;
        uint32_t _reserved;
    } by_index;
    uint64_t as_uint64; /* Passed directly with TOWER_EINJ_TARGET_RAW */
} tower_error_inj_param_t;

static_assert(sizeof(tower_error_inj_param_t) == sizeof(uint64_t), "'tower_error_inj_param_t' is not the expected size!");

#pragma pack(pop)

/*--------- Function Prototypes ----------*/

/**
 * @brief Handle Tower FMU records
 *        This function will iteratively (in an index ordered basis) retrieve
 *        FMU records found within the respective Tower and take appropriate
 *        actions depending on the record.
 *
 *        For records found to be fatal, a bugcheck will be invoked after all
 *        other records have been logged. If more than one record is fatal,
 *        only the first is reported on the bugcheck.
 *
 * @param[in] tower_id - TOWER_INSTANCE instance id
 * @param[in] die      - DIE_INSTANCE instance id
 * @param[in] base     - UINTPTR_T base address to use for the tower
 *
 * @retval SILIBS_STATUS_T status
 */
silibs_status_t tower_fmu_handler(TOWER_INSTANCE tower_id, DIE_INSTANCE die, uintptr_t base);

/**
 *  @brief Tower Error Injection Callback
 *         Perform error spoof/injection of the requested EINJ payload.
 *
 *  @param[in] einj_payload - Injection payload
 *
 *  @param[in] ctx - Unused
 *
 *  @return   ACPI_EINJ_SUCCESS         - Error injection was successful
 *            ACPI_EINJ_INVALID_ACCESS  - Invalid injection payload buffer or parameters
 *            ACPI_EINJ_UNKNOWN_FAILURE - Unknown failure occurred during error injection
 */
acpi_einj_cmd_status_t tower_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx);
