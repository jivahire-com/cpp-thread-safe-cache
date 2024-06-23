//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sds_api_impl.c
 * Implements the SDS service API functions.
 */

/*------------- Includes -----------------*/
#include "sds_api.h"

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
int32_t sds_read(PDFWK_INTERFACE_HEADER pInterface, void* buffer, size_t buffer_size, size_t* processed_bytes)
{
    sds_service_request_t request = {};
    request.header.RequestType = SDS_IO_REQUEST_READ_SYNC;
    request.input.buffer = buffer;
    request.input.byte_count = buffer_size;

    int32_t status = DfwkInterfaceSendSync(pInterface, &request.header);

    if (status == DFWK_SUCCESS)
    {
        if (processed_bytes != NULL)
        {
            *processed_bytes = request.output.processed_bytes;
        }
    }
    else
    {
        SDS_LOG_CRIT("Failed to send SDS read request. Error code: %d", (int)status);
        if (processed_bytes != NULL)
        {
            *processed_bytes = 0;
        }
    }

    return status;
}

int32_t sds_write(PDFWK_INTERFACE_HEADER pInterface, void* buffer, size_t buffer_size, size_t* processed_bytes)
{
    sds_service_request_t request = {};
    request.header.RequestType = SDS_IO_REQUEST_WRITE_SYNC;
    request.input.buffer = buffer;
    request.input.byte_count = buffer_size;

    int32_t status = DfwkInterfaceSendSync(pInterface, &request.header);

    if (status == DFWK_SUCCESS)
    {
        if (processed_bytes != NULL)
        {
            *processed_bytes = request.output.processed_bytes;
        }
    }
    else
    {
        SDS_LOG_CRIT("Failed to send SDS write request. Error code: %d", (int)status);
        if (processed_bytes != NULL)
        {
            *processed_bytes = 0;
        }
    }

    return status;
}
