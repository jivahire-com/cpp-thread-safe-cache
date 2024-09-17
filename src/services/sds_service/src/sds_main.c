//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file sds_main.c
 * Implements the primary driver interface of sds service.
 */

/*------------- Includes -----------------*/
#include "sds_api.h"
#include "sds_configuration.h"
#include "sds_init.h"

#include <DfwkDriver.h>
#include <DfwkHost.h>
#include <FpFwAssert.h>
#include <FpFwSpinLock.h>
#include <FpFwUtils.h>
#include <atu_api.h>
#include <bug_check.h>
#include <fpfw_status.h>
#include <kng_error.h>
#include <shared_sds_def.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define ATU_AP_SDS_ADDRESS   (ATU_AP_ARSM_ADDRESS + SDS_AP_ADDRESS_BASE)
#define SDS_ALLOWED_MAX_SIZE (SDS_SIZE_MAX - sizeof(FPFW_SPINLOCK))

static sds_current_context_t current_SDS_layout_context = {0};
static volatile PFPFW_SPINLOCK sharedMemoryLock = (PFPFW_SPINLOCK)(ATU_AP_ARSM_ADDRESS + SDS_ALLOWED_MAX_SIZE);
extern psds_service_interface_t sds_interface;

/*------------- Functions ----------------*/
sds_config_t* retrieve_sds_config_info()
{
    static sds_region_desc_t sds_region_descriptor[PLATFORM_SDS_REGION_COUNT] = {[PLATFORM_SDS_REGION_ARSM_DIE0] = {
                                                                                     .base = (void*)ATU_AP_SDS_ADDRESS,
                                                                                     .size = SDS_ALLOWED_MAX_SIZE,
                                                                                 }};

    static sds_config_t platform_sds_config = {
        .regions = sds_region_descriptor,
        .region_count = PLATFORM_SDS_REGION_COUNT,
    };

    return &platform_sds_config;
}

int32_t get_sds_block_info(uint32_t sds_block_id, sds_block_header_t* p_sds_block_header_info, volatile char** current_sds_block_base_addr)
{
    volatile sds_block_header_t* p_current_sds_block_header;
    volatile sds_region_header_t* p_sds_region_desc_addr;
    volatile char* p_sds_region_base;
    uint32_t offset;

    sds_config_t* sds_region_config_info = retrieve_sds_config_info();
    FPFW_RUNTIME_ASSERT(sds_region_config_info != NULL);

    for (uint32_t region_idx = 0; region_idx < sds_region_config_info->region_count; region_idx++)
    {
        offset = sizeof(sds_region_header_t);

        p_sds_region_base = (volatile char*)sds_region_config_info->regions[region_idx].base;
        p_sds_region_desc_addr = (volatile sds_region_header_t*)p_sds_region_base;

        for (uint32_t sds_block_idx = 0; sds_block_idx < p_sds_region_desc_addr->block_count; sds_block_idx++)
        {
            p_current_sds_block_header = (volatile sds_block_header_t*)(p_sds_region_base + offset);

            offset += p_current_sds_block_header->sds_block_size;
            offset += sizeof(sds_block_header_t);

            if (p_sds_region_desc_addr->region_size < offset)
            {
                FPFW_RUNTIME_ASSERT(false);
            }

            if (p_current_sds_block_header->sds_block_id == sds_block_id)
            {
                if (current_sds_block_base_addr != NULL)
                {
                    *current_sds_block_base_addr =
                        ((volatile char*)p_current_sds_block_header + sizeof(sds_block_header_t));
                }

                *p_sds_block_header_info = *p_current_sds_block_header;
                return KNG_SUCCESS;
            }
        }
    }

    return KNG_E_FAIL;
}

bool check_sds_block_presence(uint32_t sds_module_id)
{
    int status;
    sds_block_header_t sds_block_header_info;

    status = get_sds_block_info(sds_module_id, &sds_block_header_info, NULL);
    return KNG_SUCCEEDED(status);
}

int32_t create_sds_block_internal(psds_service_request_t sds_request)
{
    uint32_t aligned_requested_size;
    volatile sds_region_header_t* sds_region_desc_addr;
    volatile sds_block_header_t* current_block_header = NULL;
    sds_config_t* sds_region_config_info = retrieve_sds_config_info();
    uint32_t* current_sds_free_size =
        &(current_SDS_layout_context.sds_regions[sds_request->sds_module_region_id].free_mem_size);
    volatile char** current_sds_free_addr =
        &(current_SDS_layout_context.sds_regions[sds_request->sds_module_region_id].free_mem_base);

    aligned_requested_size = SDS_ALIGN(sds_request->sds_module_size);

    if ((aligned_requested_size + sizeof(*current_block_header)) > *current_sds_free_size)
    {
        return KNG_E_OUTOFMEMORY;
    }

    // create new sds block with module id and requested size
    current_block_header = (volatile sds_block_header_t*)(*current_sds_free_addr);
    current_block_header->sds_block_id = sds_request->sds_module_id;
    current_block_header->sds_block_size = aligned_requested_size;
    current_block_header->valid = true;
    *current_sds_free_addr += sizeof(*current_block_header);
    *current_sds_free_size -= sizeof(*current_block_header);

    for (uint32_t idx = 0; idx < aligned_requested_size; idx++)
    {
        (*current_sds_free_addr)[idx] = '\0';
    }

    *current_sds_free_addr += aligned_requested_size;
    *current_sds_free_size -= aligned_requested_size;

    /* Increment the structure count within the region descriptor */
    sds_region_desc_addr =
        (volatile sds_region_header_t*)(sds_region_config_info->regions[sds_request->sds_module_region_id].base);
    sds_region_desc_addr->block_count++;

    return KNG_SUCCESS;
}

int32_t create_sds_region(const sds_region_desc_t* region_config, unsigned int region_idx)
{
    volatile sds_region_header_t* region_desc = (volatile sds_region_header_t*)region_config->base;

    if (region_config->size < SDS_MIN_REGION_SIZE)
    {
        return KNG_E_INVALIDARG;
    }

    // Check if this is reinitialize request or not.
    if (region_desc->signature != REGION_SIGNATURE)
    {
        // Initialize the region descriptor
        region_desc->signature = REGION_SIGNATURE;
        region_desc->version_major = SUPPORTED_VERSION_MAJOR;
        region_desc->version_minor = SUPPORTED_VERSION_MINOR;
        region_desc->region_size = region_config->size;
        region_desc->block_count = 0;

        current_SDS_layout_context.sds_regions[region_idx].free_mem_size =
            region_config->size - sizeof(sds_region_header_t);
        current_SDS_layout_context.sds_regions[region_idx].free_mem_base =
            (volatile char*)region_config->base + sizeof(sds_region_header_t);
    }
    else
    {
        volatile sds_block_header_t* sds_current_block_header = NULL;

        // Reinitialize the region descriptor
        region_desc->region_size = region_config->size;

        uint32_t current_mem_usage = (uint32_t)sizeof(sds_region_header_t);

        for (uint32_t sds_block_idx = 0; sds_block_idx < region_desc->block_count; sds_block_idx++)
        {
            sds_current_block_header =
                (volatile sds_block_header_t*)((volatile char*)region_config->base + current_mem_usage);

            current_mem_usage += (uint32_t)sizeof(sds_block_header_t);
            current_mem_usage += sds_current_block_header->sds_block_size;
        }

        if (region_desc->region_size < current_mem_usage)
        {
            return KNG_E_OUTOFMEMORY;
        }

        current_SDS_layout_context.sds_regions[region_idx].free_mem_size = region_config->size - current_mem_usage;
        current_SDS_layout_context.sds_regions[region_idx].free_mem_base =
            (volatile char*)region_config->base + current_mem_usage;
    }

    return KNG_SUCCESS;
}

int32_t create_sds_block(sds_service_request_t* p_sds_request)
{
    FPFW_RUNTIME_ASSERT(p_sds_request != NULL);

    if (!check_sds_block_presence(p_sds_request->sds_module_id))
    {
        return create_sds_block_internal(p_sds_request);
    }

    return KNG_SUCCESS;
}

int32_t read_sds_block(sds_service_request_t* p_sds_request)
{
    volatile char* current_sds_block_free_addr;
    sds_block_header_t sds_block_header_info;

    if (get_sds_block_info(p_sds_request->sds_module_id, &sds_block_header_info, &current_sds_block_free_addr) != KNG_SUCCESS)
    {
        SDS_LOG_CRIT("No Matching SDS Block for %d", (int)p_sds_request->sds_module_id);
        return KNG_E_NOT_FOUND;
    }

    if (sds_block_header_info.sds_block_size < p_sds_request->io.byte_count)
    {
        SDS_LOG_CRIT("Beyond Memory Capacity Access Not Allowed(Total : %d, Requested : %d)",
                     (int)sds_block_header_info.sds_block_size,
                     (int)p_sds_request->io.byte_count);
        return KNG_E_INVALIDARG;
    }

    for (uint32_t idx = 0; idx < p_sds_request->io.byte_count; idx++)
    {
        ((char*)p_sds_request->io.buffer)[idx] = current_sds_block_free_addr[idx];
    }

    return KNG_SUCCESS;
}

int32_t write_sds_block(sds_service_request_t* p_sds_request)
{
    volatile char* current_sds_block_free_addr;
    sds_block_header_t sds_block_header_info;

    if (get_sds_block_info(p_sds_request->sds_module_id, &sds_block_header_info, &current_sds_block_free_addr) != KNG_SUCCESS)
    {
        SDS_LOG_CRIT("No Matching SDS Block for %d", (int)p_sds_request->sds_module_id);
        return KNG_E_NOT_FOUND;
    }

    if (sds_block_header_info.sds_block_size < p_sds_request->io.byte_count)
    {
        SDS_LOG_CRIT("Beyond Memory Capacity Access Not Allowed(Total : %d, Requested : %d)",
                     (int)sds_block_header_info.sds_block_size,
                     (int)p_sds_request->io.byte_count);
        return KNG_E_INVALIDARG;
    }

    for (uint32_t idx = 0; idx < p_sds_request->io.byte_count; idx++)
    {
        current_sds_block_free_addr[idx] = ((const char*)p_sds_request->io.buffer)[idx];
    }

    return KNG_SUCCESS;
}

void sds_service_dispatch(PDFWK_ASYNC_REQUEST_HEADER p_request, void* p_context)
{
    FPFW_UNUSED(p_context);

    switch (p_request->RequestType)
    {
    default:
        SDS_LOG_WARN("SDS service doesn't support async request");
        FPFW_RUNTIME_ASSERT(false);
        break;
    }
}

int32_t sds_service_dispatch_sync(PDFWK_SYNC_REQUEST_HEADER p_request)
{
    FPFW_RUNTIME_ASSERT(p_request != NULL);

    int32_t status = KNG_SUCCESS;
    psds_service_request_t p_sds_request = (psds_service_request_t)p_request;

    FPFwSpinLockAcquire(sharedMemoryLock);

    switch (p_request->RequestType)
    {
    case SDS_IO_REQUEST_CREATION_SYNC: {
        status = create_sds_block(p_sds_request);
        break;
    }
    case SDS_IO_REQUEST_READ_SYNC: {
        status = read_sds_block(p_sds_request);
        break;
    }
    case SDS_IO_REQUEST_WRITE_SYNC: {
        status = write_sds_block(p_sds_request);
        break;
    }
    default:
        status = KNG_E_INVALIDARG;
        break;
    }

    FPFwSpinLockRelease(sharedMemoryLock);

    return status;
}

void sds_interface_init(psds_service_t p_device, psds_service_interface_t p_interface)
{
    DfwkInterfaceInitialize(&p_interface->header, &p_device->header, &p_device->default_queue, sds_service_dispatch_sync);
    DfwkInterfaceOpen(&p_interface->header, &p_device->header);
    p_interface->p_device = p_device;
    sds_interface = p_interface;
}

void sds_init(psds_service_t p_device, PDFWK_SCHEDULE p_schedule)
{
    FPFwSpinLockInitialize(sharedMemoryLock);

    DfwkDeviceInitialize(&p_device->header, p_schedule);
    DfwkQueueInitialize(&p_device->default_queue, &p_device->header, sds_service_dispatch, &p_device->header, DfwkQueueType_SerializedDispatch);

    sds_config_t* sds_config = retrieve_sds_config_info();

    // Check SDS Configuration is valid
    FPFW_RUNTIME_ASSERT(sds_config != NULL);
    FPFW_RUNTIME_ASSERT(sds_config->region_count > 0);

    int32_t status = KNG_SUCCESS;
    for (uint32_t region_idx = 0; region_idx < sds_config->region_count; region_idx++)
    {
        status = create_sds_region(&(sds_config->regions[region_idx]), region_idx);
        BUG_ASSERT_PARAM(status == KNG_SUCCESS, status, KNG_SUCCESS);
    }
}