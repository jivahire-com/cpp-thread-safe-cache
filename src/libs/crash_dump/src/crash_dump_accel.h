//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file crash_dump_accel.h
 * APIs for registering crash dump accel to the crash dump
 */
#pragma once

/*---------- Nested Includes -------------*/
#include <crash_dump.h> // for CD_GUID

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/**
 * @brief Copies accel device crashdump file from accel DTCM to DDR
 * This API should be registered with CD framework to be called post
 * crashdump collection.
 *
 * @param ctx Accel device ID such as SDM or CDED
 */
void crash_dump_copy_accel_cd_file(void *ctx);

/**
 * @brief Extracts the crash timestamp from the crash information chunk in the Accel CD
 * 
 * @param p_dump_crash_info Pointer to the crash information chunk
 * @return Extracted crash timestamp
 */
uint64_t get_timestamp_from_cd(DUMP_CHUNK_CRASH_INFORMATION* p_dump_crash_info);

/**
 * @brief Updates the crash timestamp in the crash information chunk in the accel CD
 * 
 * @param p_dump_crash_info Pointer to the crash information chunk
 * @param utc_crash_time_ms Timestamp to be updated
 */
void update_timestamp_in_cd(DUMP_CHUNK_CRASH_INFORMATION* p_dump_crash_info, uint64_t utc_crash_time_ms);

/**
 * @brief Sets up accel crash dump descriptors for each non-isolated accelerator
 *
 * @param context Unused context pointer (for callback compatibility)
 */
void crash_dump_setup_accel_descriptors(void* context);
