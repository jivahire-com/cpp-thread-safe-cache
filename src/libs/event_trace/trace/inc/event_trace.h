//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace.h
 * This file is used for client to create event definitions.
 */

#pragma once

/*----------- Nested includes ------------*/

#include <FpFwMacroHelpers.h>             // IWYU pragma: export
#include <IFpFwEventTracingDefines.h>     // IWYU pragma: export
#include <IFpFwEventTracingEvents.h>      // IWYU pragma: export
#include <IFpFwEventTracingGeneration.h>  // IWYU pragma: export

/*-- Symbolic Constant Macros (defines) --*/

// Set event tracing masking level based on debug/release build
// NOTE: Refer 'FPFW_ET_LEVEL_MASK' for more info
#ifndef NDEBUG
    #define ET_LEVEL_MASK_ALL \
        (FPFW_ET_LEVEL_MASK_ALWAYS | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_WARNING | \
         FPFW_ET_LEVEL_MASK_INFO | FPFW_ET_LEVEL_MASK_VERBOSE | FPFW_ET_LEVEL_MASK_DEBUG)
#else
    #define ET_LEVEL_MASK_ALL \
        (FPFW_ET_LEVEL_MASK_ALWAYS | FPFW_ET_LEVEL_MASK_FATAL | FPFW_ET_LEVEL_MASK_ERROR | FPFW_ET_LEVEL_MASK_WARNING)
#endif

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*--------- Function Prototypes ----------*/