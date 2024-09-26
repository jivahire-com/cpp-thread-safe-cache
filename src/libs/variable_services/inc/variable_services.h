//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services.h
 * Header file for variable services public APIs
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stddef.h>       // for size_t
#include <stdint.h>       // for uintptr_t
#include <common_types.h>
#include <fpfw_icc_base.h> // for FPFW_ICC_BASE

/*-- Symbolic Constant Macros (defines) --*/
//! Variable service attributes
#define EFI_VARIABLE_NON_VOLATILE                           0x00000001UL
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                     0x00000002UL
#define EFI_VARIABLE_RUNTIME_ACCESS                         0x00000004UL
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD                  0x00000008UL
#define EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS             0x00000010UL
#define EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS  0x00000020UL
#define EFI_VARIABLE_APPEND_WRITE                           0x00000040UL

/*-------------- Typedefs ----------------*/
//! Do we need to maintain a list of shared memory regions for different variables here?
//! Do we have a use case for query variable or get next variable from MSCP?

/**
 * @brief Structure to hold shared memory region for variable service
 * Each variable will have it's own memory section carved out.
 * The memory section will be used to pass data between HSP and MSCP
 * The user of the API is responsible for verifying the memory region is valid.
 */
typedef struct _variable_service_shared_mem
{
    uintptr_t payload_base; //! starting address of the shared memory region for the variable
    size_t max_payload_size; //! size of the shared memory region for the variable
} var_service_shared_mem_t;

/**
 * @brief Structure to hold the parameters for GET/SET variable service request
 * 
 */
typedef struct _variable_service_req_params
{
    uint16_t *variable_name_ptr; //! Name of the variable
    size_t variable_name_size; //! Size of the variable name
    guid_t vendor_namespace_guid; //! Vendor namespace GUID
    size_t data_size; //! Size of the data, must be within the max_payload_size
    /**
     * @brief Caller's data associated with the variable name
     * copied to the shared memory region for set variable
     * copied from the shared memory region for get variable
     */
    uint8_t *data; 
    union {
        uint32_t attributes; //! Attributes of the variable, applicable & required only for SET variable
        uint32_t attributes_size; //! Size of the attributes, applicable & optional only for GET variable
    };
} var_service_req_params_t;

/*--------- Function Prototypes ----------*/

/**
 * @brief  Sync API to read a variable from HSP over Hsp Mbox. The response can be 
 * fetched from the shared memory region provided by the caller.
 * 
 * @note Blocking call, this API must be invoked only during the initialization. 
 * 
 * @param mem_ctx  Shared memory context for the variable
 * @param req_params Request parameters for the variable
 */
void variable_service_sync_get_variable(fpfw_icc_base_ctx_t* hsp_icc_ctx, var_service_shared_mem_t *mem_ctx, var_service_req_params_t *req_params);

/**
 * @brief  Sync API to write a variable & send to HSP over Hsp Mbox.
 * 
 * @note Blocking call, this API must be invoked only during the initialization.
 * 
 * @param mem_ctx  Shared memory context for the variable
 * @param req_params  Request parameters for the variable
 */
void variable_service_sync_set_variable(fpfw_icc_base_ctx_t* hsp_icc_ctx, var_service_shared_mem_t *mem_ctx, var_service_req_params_t *req_params);

