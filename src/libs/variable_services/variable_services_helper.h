//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_helper.h
 * Internal Header file for common helper APIs for sync & async implementation of variable services
 */

#pragma once

/*----------- Nested includes ------------*/
#include "variable_services.h"

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
            printf(fmt, ## args);     \
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

/**
 * @brief Sets the shared memory in use bit to true if not already set
 */
bool variable_service_set_shared_mem_in_use(var_service_req_ctx_t* var_serv_ctx);

/**
 * @brief Get the variable service payload address from the base address provided
 * in variable_service_shared_mem_t structure.
 * This address will be sent to HSP via mbox
 * 
 * @param payload_base shared memory base address
 * @return uint64_t variable service payload address
 */
uint64_t get_variable_serv_payload_address(uintptr_t payload_base);

/**
 * @brief Debug print before sending the mbox request
 * 
 * @param var_serv_ctx 
 */
void debug_print_pre_mbox_send(var_service_req_ctx_t* var_serv_ctx);

/**
 * @brief Debug print after receiving the mbox response
 * @param var_serv_ctx 
 * @param output_size_bytes 
 * @param status 
 */
void debug_print_post_mbox_recv(var_service_req_ctx_t* var_serv_ctx, size_t output_size_bytes, fpfw_status_t status);