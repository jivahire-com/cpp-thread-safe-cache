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

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
int32_t sds_io_request(PDFWK_INTERFACE_HEADER p_interface, void* buffer, size_t buffer_size, sds_api_type_t requestType)
{
    BUG_ASSERT(buffer != NULL);

    sds_service_request_t request;
    request.header.RequestType = requestType;
    request.input.buffer = buffer;
    request.input.byte_count = buffer_size;

    int32_t status = DfwkInterfaceSendSync(p_interface, &request.header);

    if (status != DFWK_SUCCESS)
    {
        SDS_LOG_CRIT("Failed to send SDS request. Op Code: %d, Error code: %d", (int)requestType, (int)status);
    }

    return status;
}

int32_t sds_block_creation(PDFWK_INTERFACE_HEADER p_interface, const char* module_name, uint32_t request_size, uint32_t region_id)
{
    BUG_ASSERT(module_name != NULL);
    BUG_ASSERT(request_size != 0);

    sds_config_t* sds_config = retrieve_sds_config_info();

    BUG_ASSERT(sds_config != NULL);
    BUG_ASSERT(region_id <= sds_config->region_count);
    BUG_ASSERT(SDS_MIN_REGION_SIZE <= request_size);

    sds_service_interface_t* p_sds_interface = (sds_service_interface_t*)p_interface;
    p_sds_interface->module_name = module_name;
    p_sds_interface->request_size = request_size;
    p_sds_interface->region_id = region_id;

    sds_service_request_t request = {};
    request.header.RequestType = SDS_IO_REQUEST_CREATION_SYNC;

    int32_t status = DfwkInterfaceSendSync(p_interface, &request.header);

    if (status != DFWK_SUCCESS)
    {
        SDS_LOG_CRIT("Failed to create SDS block. Error code: %d", (int)status);
    }

    return status;
}

int32_t sds_block_read(PDFWK_INTERFACE_HEADER p_interface, void* buffer, size_t buffer_size)
{
    return sds_io_request(p_interface, buffer, buffer_size, SDS_IO_REQUEST_READ_SYNC);
}

int32_t sds_block_write(PDFWK_INTERFACE_HEADER p_interface, void* buffer, size_t buffer_size)
{
    return sds_io_request(p_interface, buffer, buffer_size, SDS_IO_REQUEST_WRITE_SYNC);
}
