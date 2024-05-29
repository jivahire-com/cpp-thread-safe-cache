//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_i.h
 * Header containing internal definitions for power service
 */

#pragma once

/*----------- Nested includes ------------*/
#include <FpFwAssert.h>
#include <stdbool.h>
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
#define MODULE_NAME "[PWRSVC] "
#define NEWLINE     "\n"

#define POWER_LOG_INFO(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define POWER_LOG_WARN(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)
#define POWER_LOG_CRIT(fmt, ...) printf(MODULE_NAME fmt NEWLINE, ##__VA_ARGS__)

#ifndef MIN
    #define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif
#ifndef MAX
    #define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/* other helper macros */
#define FLOAT_TO_UNSIGNED(x) ((unsigned)(x + 0.5f))

/* temporary */
#define BUG_CHECK(code, param1, param2)            FPFW_RUNTIME_ASSERT(false)
#define BUG_ASSERT_PARAM(expected, param1, param2) FPFW_RUNTIME_ASSERT(expected)

// Helpers for DTS Coefficient values taken from fuses:
// temp as these should go in pvt_struct.h; use only if not already defined
#define DTS_K_COEFF_FUSED_TEMP(fused_k) (-1.0F * (float)fused_k)
#define DTS_Y_COEFF_FUSED_TEMP(fused_y) ((float)fused_y)
#ifndef TEMP2DOUT_FUSED
    #define TEMP2DOUT_FUSED(t, fused_k, fused_y) \
        (uint32_t)(16384.0F * (t - DTS_K_COEFF_FUSED_TEMP(fused_k)) / DTS_Y_COEFF_FUSED_TEMP(fused_y))
#endif
#ifndef DOUT2TEMP_FUSED
    #define DOUT2TEMP_FUSED(dout, fused_k, fused_y) \
        (dout / 16384.0F * DTS_Y_COEFF_FUSED_TEMP(fused_y) + DTS_K_COEFF_FUSED_TEMP(fused_k))
#endif

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif