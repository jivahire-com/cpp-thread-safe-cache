//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file rtosmon_service.h
 *
 * Public header for the rtosmon_service service
 *
 */

#pragma once

/*----------------------------- Nested includes -----------------------------*/

#include <fpfw_status.h> // for fpfw_status_t

/*------------------- Symbolic Constant Macros (defines) --------------------*/

/*-------------------------------- Typedefs ---------------------------------*/

/*------------------- Declarations (Statics and globals) --------------------*/

/*--------------------------- Function Prototypes ---------------------------*/

/**
 * @brief This function initializes RTOSMON and creates the event trace thread
 *
 * @return FPFW_STATUS_SUCCESS/FPFW_STATUS_FAIL based on success/failure
 */
fpfw_status_t rtosmon_service_init();

