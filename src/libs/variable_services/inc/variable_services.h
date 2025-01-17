//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services.h
 * Header file for variable services public APIs
 */

#pragma once

/*----------- Nested includes ------------*/
#include <FpFwLock.h>
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
/**
 * @brief  Enum to define the type of operation to be performed on the variable
 *
 */
typedef enum variable_service_operation_type
{
    SYNC_GET_VARIABLE = 0,
    SYNC_SET_VARIABLE,
    ASYNC_GET_VARIABLE,
    ASYNC_SET_VARIABLE,
    OPERATION_TYPE_MAX,
} variable_service_operation_type_t;

/**
 * @brief struct for variable service attributes, applicable for set variable requests only
 */
typedef union _variable_service_attributes
{
    struct {
        uint32_t non_volatile : 1;                           //! Non-volatile attribute
        uint32_t bootservice_access : 1;                     //! Boot service access attribute
        uint32_t runtime_access : 1;                         //! Runtime access attribute
        uint32_t hardware_error_record : 1;                  //! Hardware error record attribute
        uint32_t authenticated_write_access : 1;             //! Authenticated write access attribute
        uint32_t time_based_authenticated_write_access : 1;  //! Time-based authenticated write access attribute
        uint32_t append_write : 1;                           //! Append write attribute
        uint32_t reserved : 25;                              //! Reserved bits
    }bitfield;
    uint32_t as_uint32;
} variable_service_attributes_t;

/**
 * @brief Structure to hold shared memory region for variable service
 * Each variable will have it's own memory section carved out.
 * The memory section will be used to pass data between HSP and MSCP
 * The user of the API is responsible for verifying the memory region is valid.
 * 
 * @note Do not modify the shared memory ctx until the variable service API returns.
 * The caller must ensure this shared mem ctx is valid for the lifetime of the variable service API call.
 */
typedef struct _variable_service_shared_mem
{
    uintptr_t payload_base; //! mandatory, starting address of the shared memory region for the variable, 32 bits, accessible by MSCP (atu mapped if applicable)
    size_t max_payload_size; //! mandatory, size of the shared memory region for the variable
} var_service_shared_mem_t;

/**
 * @brief Structure to hold the parameters for GET/SET variable service request
 * 
 * @note The params are copied internally, the caller can free the memory after the API call.
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
        variable_service_attributes_t attributes; //! Attributes of the variable, applicable & required only for SET variable
        uint32_t attributes_size; //! Size of the attributes, applicable & optional only for GET variable
    };
} var_service_req_params_t;

/**
 * @brief Forward declaration of the variable service request context
 */
struct _variable_service_req_ctx;

/**
 * @brief Async callback called when the variable service request is complete & the response is posted in the shared memory region
 * 
 * @param context Caller's context passed to the callback
 * @param var_serv_ctx Variable service context, output parameter
 * @param data_start_ptr Pointer to the data in the shared memory region, output parameter
 * @param data_size Size of the data in the shared memory region, output parameter
 */
typedef void (*variable_service_req_complete_notify)(void* context, struct _variable_service_req_ctx *var_serv_ctx, uint8_t* data_start_ptr, size_t data_size);

/**
 * @brief Variable service request context, the user must allocate memory for this structure
 * however it will be internally populated by the variable service APIs.
 * 
 * @note Each context associated with a particular shared memory region
 * Only one active request allowed per context i.e Only async sequential requests are allowed.
 * Concurrent request using the same context will return error code.
 */
typedef struct _variable_service_req_ctx
{
    fpfw_icc_base_send_recv_req_t icc_req; //! ICC request object, variable services needs this mem from the caller, leave unpopulated
    var_service_shared_mem_t shared_mem; //! Shared memory region for variable service
    var_service_req_params_t req_params; //! populated by the caller, original request params
    variable_service_req_complete_notify callback; //! callback to notify the caller
    void* context; //! context to be passed to the callback
    variable_service_operation_type_t operation_type; //! GET/SET variable
    FPFW_LOCK var_serv_lock;                        //! lock to protect ctx
    bool is_initialized; //! 0: not initialized, 1: initialized
    uint32_t async_req_result; //! result of the async request
}var_service_req_ctx_t;

/*--------- Function Prototypes ----------*/

/**
 * @brief API to initialized variable services in runtime init
 * Store icc base ctx used for subsequent variable service calls
 * 
 * @param icc_ctx Supplied in runtime init, hsp mbx icc base ctx
 */
void variable_service_init(fpfw_icc_base_ctx_t* icc_ctx);

/**
 * @brief  API to initialize the shared memory region for the variable service
 * Called once per unique shared memory region for a variable.
 * 
 * @param var_serv_ctx  Variable service context
 * @param mem_ctx  Shared memory context for the variable
 * @return int32_t  KNG_SUCCESS if successful, else KNG_E_INVALIDARG
 */
int32_t variable_service_initialize_ctx(var_service_req_ctx_t *var_serv_ctx, var_service_shared_mem_t *mem_ctx);

/**
 * @brief  Sync API to read a variable from HSP over Hsp Mbox. The response can be 
 * fetched from the shared memory region provided by the caller.
 * 
 * @note Blocking call, this API must be invoked only during the initialization. 
 * 
 * @param var_serv_ctx  Variable service context
 * @param req_params Request parameters for the variable
 * @return KNG_SUCCESS if successful, else KNG_E_NOT_FOUND if the variable is not found
 */
int32_t variable_service_sync_get_variable(var_service_req_ctx_t *var_serv_ctx, var_service_req_params_t *req_params);

/**
 * @brief  Sync API to write a variable & send to HSP over Hsp Mbox.
 * 
 * @note Blocking call, this API must be invoked only during the initialization.
 * 
 * @param var_serv_ctx  Variable service context
 * @param req_params  Request parameters for the variable
 */
void variable_service_sync_set_variable(var_service_req_ctx_t *var_serv_ctx, var_service_req_params_t *req_params);

/**
 * @brief API to free the context allocated by the caller for get variable service requests
 * Applicable for both sync & async Get Variable requests
 * 
 * @param var_serv_ctx 
 */
void variable_service_unlock_get_var_ctx(var_service_req_ctx_t* var_serv_ctx);

/**
 * @brief Async API to read a variable from HSP over Hsp Mbox. The response can be
 * fetched from the shared memory region provided by the caller. Pointer to the data is
 * also provided in the callback registered by the caller.
 * 
 * @param var_serv_ctx  Variable service context
 * @param req_params  Request parameters for the variable
 * @param callback  Callback to notify the caller that the request is complete & the response is posted in the shared memory region
 * @param context  Optional, Caller's Context to be passed to the callback
 * @return int32_t  KNG_SUCCESS if successful, else KNG_E_INVALIDARG
 */
int32_t variable_service_async_get_variable(var_service_req_ctx_t *var_serv_ctx, var_service_req_params_t *req_params, variable_service_req_complete_notify callback, void* context);

/**
 * @brief  Async API to write a variable & send to HSP over Hsp Mbox. The response can be
 * fetched from the shared memory region provided by the caller. Pointer to the data is
 * also provided in the callback registered by the caller.
 * 
 * @param var_serv_ctx  Variable service context
 * @param req_params  Request parameters for the variable
 * @param callback  Callback to notify the caller that the request is complete & the response is posted in the shared memory region
 * @param context  Optional, Caller's Context to be passed to the callback
 * @return int32_t  KNG_SUCCESS if successful, else KNG_E_INVALIDARG
 */
int32_t variable_service_async_set_variable(var_service_req_ctx_t *var_serv_ctx, var_service_req_params_t *req_params, variable_service_req_complete_notify callback, void* context);