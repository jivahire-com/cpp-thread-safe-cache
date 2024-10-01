//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_helper.h
 * Internal Header file for common helper APIs for sync & async implementation of variable services
 */

#pragma once

/*----------- Nested includes ------------*/
#include <fpfw_icc_base.h> // for FPFW_ICC_BASE
#include <inttypes.h>     // for uint32_t
#include <stdint.h>       // for uintptr_t

/*-- Symbolic Constant Macros (defines) --*/
//! Uncomment for debug messages
// #define DEBUG_VAR_SERV 1

#if DEBUG_VAR_SERV
    #include <stdio.h>
    #define DEBUG_PRINT(fmt, args...) \
        do                            \
        {                             \
            printf(fmt, ##args);      \
        } while (0)
#else
    #define DEBUG_PRINT(fmt, args...) \
        do                            \
        {                             \
        } while (0)
#endif

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/**
 * @brief API to verify the caller provided attributes of the variable
 * Applicable for SET variable.
 * 
 * @param attributes caller provided attributes
 * @return int32_t KNG_SUCCESS if attributes are valid, else KNG_E_INVALIDARG or KNG_E_NOTIMPL
 */
int32_t variable_services_check_attribute(uint32_t attributes);

/**
 * @brief Get the icc base ctx object
 * 
 * @return fpfw_icc_base_ctx_t* if initialized else NULL
 */
fpfw_icc_base_ctx_t* get_icc_base_ctx(void);