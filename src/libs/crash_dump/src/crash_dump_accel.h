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
