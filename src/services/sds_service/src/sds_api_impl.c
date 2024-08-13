//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sds_api_impl.c
 * Implements the SDS service API functions.
 */

/*------------- Includes -----------------*/
#include "sds_api.h"
#include "sds_init.h"

#include <FpFwAssert.h>
#include <bug_check.h>
#include <fpfw_init.h>
#include <kng_error.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
psds_service_interface_t sds_interface = NULL;

/*------------- Functions ----------------*/
int32_t sds_io_request(psds_service_interface_t p_interface, uint32_t sds_module_id, void* buffer, size_t buffer_size, sds_api_type_t requestType)
{
    if (buffer_size == 0 || buffer == NULL)
    {
        return KNG_E_INVALIDARG;
    }

    sds_service_request_t request = {};
    request.header.RequestType = requestType;
    request.sds_module_id = sds_module_id;
    request.io.buffer = buffer;
    request.io.byte_count = buffer_size;

    int32_t status = DfwkInterfaceSendSync(&p_interface->header, &request.header);

    if (status != KNG_SUCCESS)
    {
        SDS_LOG_CRIT("Failed to send SDS request. Op Code: %d, Error code: %d", (int)requestType, (int)status);
    }

    return status;
}

int32_t sds_block_creation(uint32_t sds_module_id, uint32_t request_size, uint32_t region_id)
{
    if (request_size == 0 || request_size < SDS_MIN_REGION_SIZE)
    {
        return KNG_E_INVALIDARG;
    }

    sds_config_t* sds_config = retrieve_sds_config_info();
    BUG_ASSERT(sds_config != NULL);

    if (sds_config->region_count < region_id)
    {
        return KNG_E_INVALIDARG;
    }

    if (sds_interface == NULL)
    {
        return KNG_E_NOINTERFACE;
    }

    sds_service_request_t request = {};
    request.header.RequestType = SDS_IO_REQUEST_CREATION_SYNC;
    request.sds_module_id = sds_module_id;
    request.sds_module_size = request_size;
    request.sds_module_region_id = region_id;

    int32_t status = DfwkInterfaceSendSync(&sds_interface->header, &request.header);

    if (status != KNG_SUCCESS)
    {
        SDS_LOG_CRIT("Failed to create SDS block. Error code: %d", (int)status);
    }

    return status;
}

int32_t sds_block_read(uint32_t sds_module_id, void* buffer, size_t buffer_size)
{
    if (sds_interface == NULL)
    {
        return KNG_E_NOINTERFACE;
    }

    return sds_io_request(sds_interface, sds_module_id, buffer, buffer_size, SDS_IO_REQUEST_READ_SYNC);
}

int32_t sds_block_write(uint32_t sds_module_id, void* buffer, size_t buffer_size)
{
    if (sds_interface == NULL)
    {
        return KNG_E_NOINTERFACE;
    }

    return sds_io_request(sds_interface, sds_module_id, buffer, buffer_size, SDS_IO_REQUEST_WRITE_SYNC);
}
