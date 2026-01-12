//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file event_trace_relay_quiesce_i.h
 * Interface for event trace relay quiesce.
 */
#pragma once

/*----------------------------- Nested includes -----------------------------*/
#include <DfwkDriver.h>      // for DFWK_DEVICE_HEADER, DFWK_INTERFACE_HEADER
#include <mts_platform_definitions.h> // for mts_platform_core_id_t

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/
/**
 * @brief Event trace relay Device structure.
 *
 */
typedef struct {
    DFWK_DEVICE_HEADER Header;
    DFWK_QUEUE Queue;
} etr_device_t, *p_etr_device_t;

/**
 * @brief Event trace relay Interface structure.
 *
 */
typedef struct {
    DFWK_INTERFACE_HEADER Header;
    p_etr_device_t Device;
} etr_interface_t, *p_etr_interface_t;

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief Initialize the event trace relay driver framework
 *
 */
void event_trace_relay_dfwk_init();

/**
 * @brief Update the pending core quiesce flags with the core that has informed it has quiesced.
 *
 * @param core_id MTS ID of the core that has sent EVT quiesce indication
 */
void event_trace_relay_external_core_quiesce_update(mts_platform_core_id_t core_id);
