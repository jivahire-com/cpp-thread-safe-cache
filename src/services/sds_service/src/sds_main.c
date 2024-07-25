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
#include <fpfw_status.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static sds_current_context_t current_SDS_layout_context = {0};
static FPFW_SPINLOCK lock = 0;

/*------------- Functions ----------------*/
sds_config_t* retrieve_sds_config_info()
{
    static sds_region_desc_t sds_region_descriptor[PLATFORM_SDS_REGION_COUNT] = {
        [PLATFORM_SDS_REGION_SECURE] =
            {
                .base = (void*)SCP_SDS_SECURE_BASE,
                .size = SCP_SDS_SECURE_SIZE,
            },
        [PLATFORM_SDS_REGION_NONSECURE] =
            {
                .base = (void*)SCP_SDS_NON_SECURE_BASE,
                .size = SCP_SDS_NON_SECURE_SIZE,
            },
    };

    static sds_config_t platform_sds_config = {
        .regions = sds_region_descriptor,
        .region_count = PLATFORM_SDS_REGION_COUNT,
    };

    return &platform_sds_config;
}

uint32_t create_hash_from_string(const char* str)
{
    uint32_t hash = 5381;
    int c;

    // djb2 algorithm hash.
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
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
                return FPFW_STATUS_SUCCESS;
            }
        }
    }

    return FPFW_STATUS_FAIL;
}

bool check_sds_block_presence_byname(const char* module_name)
{
    FPFW_RUNTIME_ASSERT(module_name != NULL);

    int status;
    sds_block_header_t sds_block_header_info;

    status = get_sds_block_info(create_hash_from_string(module_name), &sds_block_header_info, NULL);
    return FPFW_STATUS_SUCCEEDED(status);
}

int32_t create_sds_block_internal(sds_service_interface_t* p_sds_interface)
{
    uint32_t required_alloc_roundup_size;
    volatile sds_region_header_t* sds_region_desc_addr;
    volatile sds_block_header_t* current_block_header = NULL;
    sds_config_t* sds_region_config_info = retrieve_sds_config_info();
    uint32_t* current_sds_free_size = &(current_SDS_layout_context.sds_regions[p_sds_interface->region_id].free_mem_size);
    volatile char** current_sds_free_addr =
        &(current_SDS_layout_context.sds_regions[p_sds_interface->region_id].free_mem_base);

    required_alloc_roundup_size = SDS_ALIGN(p_sds_interface->request_size);

    if ((required_alloc_roundup_size + sizeof(*current_block_header)) > *current_sds_free_size)
    {
        return FPFW_STATUS_OUT_OF_MEMORY;
    }

    // create new sds block with module name id and requested size
    current_block_header = (volatile sds_block_header_t*)(*current_sds_free_addr);
    current_block_header->sds_block_id = create_hash_from_string(p_sds_interface->module_name);
    current_block_header->sds_block_size = required_alloc_roundup_size;
    current_block_header->valid = true;
    *current_sds_free_addr += sizeof(*current_block_header);
    *current_sds_free_size -= sizeof(*current_block_header);

    for (uint32_t idx = 0; idx < required_alloc_roundup_size; idx++)
    {
        (*current_sds_free_addr)[idx] = '\0';
    }

    *current_sds_free_addr += required_alloc_roundup_size;
    *current_sds_free_size -= required_alloc_roundup_size;

    /* Increment the structure count within the region descriptor */
    sds_region_desc_addr =
        (volatile sds_region_header_t*)(sds_region_config_info->regions[p_sds_interface->region_id].base);
    sds_region_desc_addr->block_count++;

    return FPFW_STATUS_SUCCESS;
}

int32_t create_sds_region(const sds_region_desc_t* region_config, unsigned int region_idx)
{
    volatile sds_region_header_t* region_desc = (volatile sds_region_header_t*)region_config->base;

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
            return FPFW_STATUS_OUT_OF_MEMORY;
        }

        current_SDS_layout_context.sds_regions[region_idx].free_mem_size = region_config->size - current_mem_usage;
        current_SDS_layout_context.sds_regions[region_idx].free_mem_base =
            (volatile char*)region_config->base + current_mem_usage;
    }

    return FPFW_STATUS_SUCCESS;
}

int32_t create_sds_block(sds_service_interface_t* p_sds_interface)
{
    FPFW_RUNTIME_ASSERT(p_sds_interface != NULL);

    if (!check_sds_block_presence_byname(p_sds_interface->module_name))
    {
        return create_sds_block_internal(p_sds_interface);
    }

    return FPFW_STATUS_SUCCESS;
}

int32_t read_sds_block(sds_service_interface_t* p_sds_interface, sds_service_request_t* p_sds_request)
{
    volatile char* current_sds_block_free_addr;
    sds_block_header_t sds_block_header_info;

    if (get_sds_block_info(create_hash_from_string(p_sds_interface->module_name),
                           &sds_block_header_info,
                           &current_sds_block_free_addr) != FPFW_STATUS_SUCCESS)
    {
        SDS_LOG_CRIT("No Matching SDS Block for %s", p_sds_interface->module_name);
        return FPFW_STATUS_FAIL;
    }

    if (sds_block_header_info.sds_block_size < p_sds_request->input.byte_count)
    {
        SDS_LOG_CRIT("Beyond Memory Capacity Access Not Allowed(Total : %d, Requested : %d)",
                     (int)sds_block_header_info.sds_block_size,
                     (int)p_sds_request->input.byte_count);
        return FPFW_STATUS_OUT_OF_MEMORY;
    }

    for (uint32_t idx = 0; idx < p_sds_request->input.byte_count; idx++)
    {
        ((char*)p_sds_request->input.buffer)[idx] = current_sds_block_free_addr[idx];
    }

    return FPFW_STATUS_SUCCESS;
}

int32_t write_sds_block(sds_service_interface_t* p_sds_interface, sds_service_request_t* p_sds_request)
{
    volatile char* current_sds_block_free_addr;
    sds_block_header_t sds_block_header_info;

    if (get_sds_block_info(create_hash_from_string(p_sds_interface->module_name),
                           &sds_block_header_info,
                           &current_sds_block_free_addr) != FPFW_STATUS_SUCCESS)
    {
        SDS_LOG_CRIT("No Matching SDS Block for %s", p_sds_interface->module_name);
        return FPFW_STATUS_FAIL;
    }

    if (sds_block_header_info.sds_block_size < p_sds_request->input.byte_count)
    {
        SDS_LOG_CRIT("Beyond Memory Capacity Access Not Allowed(Total : %d, Requested : %d)",
                     (int)sds_block_header_info.sds_block_size,
                     (int)p_sds_request->input.byte_count);
        return FPFW_STATUS_OUT_OF_MEMORY;
    }

    for (uint32_t idx = 0; idx < p_sds_request->input.byte_count; idx++)
    {
        current_sds_block_free_addr[idx] = ((const char*)p_sds_request->input.buffer)[idx];
    }

    return FPFW_STATUS_SUCCESS;
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

    int32_t status = FPFW_STATUS_SUCCESS;
    psds_service_request_t p_sds_request = (psds_service_request_t)p_request;
    psds_service_interface_t p_sds_interface = (psds_service_interface_t)p_request->OwningInterface;

    FPFwSpinLockAcquire(&lock);

    switch (p_request->RequestType)
    {
    case SDS_IO_REQUEST_CREATION_SYNC: {
        status = create_sds_block(p_sds_interface);
        break;
    }
    case SDS_IO_REQUEST_READ_SYNC: {
        status = read_sds_block(p_sds_interface, p_sds_request);
        break;
    }
    case SDS_IO_REQUEST_WRITE_SYNC: {
        status = write_sds_block(p_sds_interface, p_sds_request);
        break;
    }
    default:
        FPFW_RUNTIME_ASSERT(false);
        break;
    }

    FPFwSpinLockRelease(&lock);

    return status;
}

void sds_interface_init(psds_service_t p_device, psds_service_interface_t p_interface)
{
    DfwkInterfaceInitialize(&p_interface->header, &p_device->header, &p_device->default_queue, sds_service_dispatch_sync);
    p_interface->p_device = p_device;
}

void sds_init(psds_service_t p_device, PDFWK_SCHEDULE p_schedule)
{
    FPFwSpinLockInitialize(&lock);
    DfwkDeviceInitialize(&p_device->header, p_schedule);
    DfwkQueueInitialize(&p_device->default_queue, &p_device->header, sds_service_dispatch, &p_device->header, DfwkQueueType_SerializedDispatch);

    sds_config_t* sds_config = retrieve_sds_config_info();

    // Check SDS Configuration is valid
    FPFW_RUNTIME_ASSERT(sds_config->region_count > 0);

    FPFW_RUNTIME_ASSERT(current_SDS_layout_context.sds_regions != NULL);

    for (uint32_t region_idx = 0; region_idx < sds_config->region_count; region_idx++)
    {
        create_sds_region(&(sds_config->regions[region_idx]), region_idx);
    }
}