//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file variable_services_mem.h
 * Header file for variable services shared memory format
 */

#pragma once

/*----------- Nested includes ------------*/
#include <stddef.h>       // for size_t
#include <stdint.h>       // for uintptr_t
#include <common_types.h>
#include <hsp_firmware_headers.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Typedefs ----------------*/
/**
 * @brief  Structure to hold the metadata for the shared memory region for variable service
 * This is to check if the shared memory region is in use or free for use cases where
 * multiple clients raise async variable service requests for the same variable
 * using the same memory location.
 */
typedef struct _variable_service_mem_metadata
{   
    //! Variable service memory synchronization
    union{
        struct {
            uint32_t is_in_use : 1; //! 0: free, 1: in use                            
            uint32_t reserved : 31;
        } bitfield;
        uint32_t as_uint32;
    }sync;

    //! Mailbox message for variable service
    kng_hsp_mailbox_msg msg;

    //! total size of the variable service memory region containing the actual payload
    //! excluding the metadata size
    uint32_t actual_payload_size;
} variable_service_mem_metadata_t;

/**
 * @brief Generic Structure to hold the shared memory arrangement for both get/set variable service requests
 * 
 * @details Based on an agreement between HSP and MSCP & AP for variable service communication.
 * The shared memory is 1st populated with the metadata that is managed by MSCP, followed by the
 * actual payload as seen by HSP. which contains the get/set variable structure,followed by 
 * variable name followed by actual data.
 * This struct is mapped to the "payload_base" of user provided var_service_shared_mem_t structure.
 * 
 * @note Every distinct variable must have it's own shared memory region.
 */
typedef struct _variable_service_shared_format
{
    /**
     * Carve out space in the shared memory region for the metadata
     * Managed by MSCP for enabling shared memory mgmt for async variable service communication 
     * HSP will not modify this region, only MSCP will modify this region.
     */
    variable_service_mem_metadata_t metadata;

    /**
     * Actual payload starts here:
     * 1st comes the variable service structure, followed by variable name, followed by data
     * The following is kept generic, as per hsp_mbox_set_variable/hsp_mbox_get_variable
     */
    uint32_t variable_name_size; //! Size of the variable name
    guid_t vendor_namespace_guid; //! Vendor namespace GUID
    union {
        uint32_t attributes; //! Attributes of the variable, applicable & required only for SET variable
        uint32_t attributes_size; //! Size of the attributes, applicable & optional only for GET variable
    };
    uint32_t data_size; //! Size of the data, must be within the max_payload_size

    /**
     * Next comes Variable_name, the size of which may vary from usecase to usecase, 
     * given by "variable_name_size" followed by caller's data of size "data_size" 
     * associated with the variable name.
     * Hence we have a combined flexible array entry for both variable name and data.
     */
    uint8_t variable_name_and_data[];
} variable_service_shared_mem_format_t;

