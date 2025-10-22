//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sel.h
 * Header containing definitions for the SEL module driver interface.
 */
#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_icc_base.h>  // for fpfw_icc_base_ctx_t
#include <kng_error.h> // for KNG_STATUS
#include <stdint.h> // for uint8_t, uint16_t, uint64_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
typedef enum
{
    SEL_ICC_MSCP2APS = 0,
    SEL_ICC_MSCP2MSCP = 1,
    SEL_ICC_DIE2DIE = 2,
    SEL_ICC_MAX
} sel_icc_type_t;

typedef struct __attribute__((__packed__))
{
    uint16_t record_id;     // 1-2: SEL Record ID
    uint8_t record_type;    // 3: SEL Record Type
    uint32_t timestamp;     // 4-7: Timestamp
    uint16_t generator_id;  // 8-9: Generator ID
    uint8_t evm_rev;        // 10: EVM Revision
    uint8_t sensor_type;    // 11: Sensor Type
    uint8_t sensor_number;  // 12: Sensor Number
    uint8_t event_dir_type; // 13: Event Direction Type
    uint8_t event_data[3];  // 14-16: Event Data 1, 2, 3
} sel_event_record_t;

/*--------- Function Prototypes ----------*/
/**
 * @brief Log SEL event record
 *
 * @param event_record SEL event record.
 * @return KNG_STATUS KNG_SUCCESS if succeeded, otherwise error code
 */
KNG_STATUS log_sel_event(sel_event_record_t *event_record);
