//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_platform.c
 * This file contains APIs for the DDR Manager platform-specific compatibility.
 * This file is being taken over to also provide accessor functions to aid in unit testing.
 */

/*------------- Includes -----------------*/
#include "ddr_manager.h"
#include "ddr_manager_events.h"
#include "ddr_manager_i.h"
#include "ddr_memory_map.h"

#include <FpFwUtils.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <ddrss_sdl.h>
#include <idhw.h>
#include <idsw.h>
#include <mscp_exp_rmss_memory_map.h>
#include <stdio.h>
#include <variable_services.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/

ddrss_memory_region_t* ddr_manager_get_outgoing_memory_map()
{
    return &outgoing_memory_map[0];
}

void ddr_load_shared_defect_list(void)
{
    if (idsw_get_die_id() == DIE_1)
    {
        return;
    }

    atu_map_entry_t sdl_mem_atu_map_struct = {0};
    atu_entry_attr_t sdl_atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_NS};

    sdl_mem_atu_map_struct.ap_base_address = SDL_RESERVATION_BASE;
    sdl_mem_atu_map_struct.mscp_start_address = 0;
    sdl_mem_atu_map_struct.mscp_end_address = ALIGN_UP(SDL_RESERVATION_SIZE, ATU_PAGE_SIZE) - 1;
    sdl_mem_atu_map_struct.attribute.as_uint32 = sdl_atu_root_attr.as_uint32;

    int sts = atu_map(ATU_ID_MSCP, &sdl_mem_atu_map_struct);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);

    int32_t status = get_sdl_var((uintptr_t)sdl_mem_atu_map_struct.mscp_start_address);
    if (status == KNG_E_NOT_FOUND)
    {
        // Create an empty SDL Header and copy to reserved DDR region
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_NOT_FOUND);
        copy_empty_sdl_header_to_reserved_ddr((uintptr_t)sdl_mem_atu_map_struct.mscp_start_address);
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_CREATED_EMPTY_SDL_HEADER);
    }
    else if (status == SILIBS_SUCCESS)
    {
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_FOUND_AND_LOADED_TO_DDR);
    }
    else
    {
        // Some other error occurred
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_SDL_COPY_TO_DDR_FAILED, status);
        BUG_ASSERT_PARAM(false, status, 0);
    }

    atu_unmap(ATU_ID_MSCP, &sdl_mem_atu_map_struct);
}

int32_t get_sdl_var(uintptr_t load_addr)
{
    int32_t status = SILIBS_SUCCESS;
    var_service_shared_mem_t mem_ctx = {0};
    var_service_req_ctx_t req_ctx = {0};
    var_service_req_params_t req_params = {0};
    uint16_t sdl_var_name[] = SDL_VAR_NAME;
    const guid_t sdl_var_guid[] = {SDL_VAR_GUID};

    sdl_get_mem_ctx(&mem_ctx);
    memset((void*)mem_ctx.payload_base, 0, mem_ctx.max_payload_size);

    // Initialize variable service context
    variable_service_initialize_ctx(&req_ctx, &mem_ctx);

    // Prepare GetVariable request parameters
    req_params.variable_name_ptr = (uint16_t*)sdl_var_name;
    req_params.variable_name_size = (uint32_t)sizeof(sdl_var_name);
    memcpy(&req_params.vendor_namespace_guid, sdl_var_guid, sizeof(sdl_var_guid));

    req_params.data_size = SDL_VAR_SIZE_BYTES; //  <------ Size of SDL variable without overhead/headers
    req_params.attributes_size = 0;
    req_params.data = (uint8_t*)load_addr;

    DDR_LOG_INFO("Requesting SDL variable of size %u bytes\n", req_params.data_size);

    req_params.attributes.as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    // Call variable service to get SDL variable
    DDR_LOG_INFO("Using synchronous ICC to get SDL variable\n");
    status = variable_service_sync_get_variable(&req_ctx, &req_params);
    DDR_LOG_INFO("SDL sync. get variable status: 0x%lx\n", (unsigned long)status);
    variable_service_unlock_get_var_ctx(&req_ctx);

    return status;
}

void sdl_get_mem_ctx(var_service_shared_mem_t* mem_ctx)
{
    // SCP_EXP_SDL_LOAD_BASE is an overlapping region with PCIe PHY FW load region for SDL variable service
    // SDL only needs it during early boot to load from flash to DDR reserved region
    mem_ctx->payload_base = (uintptr_t)SCP_EXP_SDL_LOAD_BASE;
    mem_ctx->max_payload_size = SCP_EXP_SDL_LOAD_SIZE;
}

// Create an empty SDL header & copy to DDR reserved region for SDL
// Caller should do its own ATU mapping
void copy_empty_sdl_header_to_reserved_ddr(uintptr_t dest_addr)
{
    MEMORY_DEFECT_LIST_HEADER empty_sdl_header = {0};
    empty_sdl_header.Signature = (uint32_t)PSHED_PI_DEFECT_LIST_SIGNATURE;
    empty_sdl_header.Version = MEMORY_DEFECT_VERSION_20;
    empty_sdl_header.Length = sizeof(MEMORY_DEFECT_LIST_HEADER);
    empty_sdl_header.DefectCount = 0;
    empty_sdl_header.Changed = 0;
    empty_sdl_header.Checksum = 0;

    empty_sdl_header.Checksum =
        (uint32_t)CalculateRemoteCheckSum16((uint32_t)&empty_sdl_header, sizeof(MEMORY_DEFECT_LIST_HEADER));

    memcpy((void*)(uint32_t)dest_addr, (void*)&empty_sdl_header, sizeof(empty_sdl_header));
}

// Writes a UEFI variable as a hand-off to OS with the address of the SDL region in DDR
void ddr_publish_sdl_addr(void)
{
    if (idsw_get_die_id() == DIE_1)
    {
        return;
    }

    var_service_req_ctx_t req_ctx = {0};
    var_service_shared_mem_t mem_ctx = {0};
    var_service_req_params_t set_var_req = {0};
    uint16_t shared_page_base_name[] = SHARED_PAGE_BASE_NAME;
    guid_t shared_page_base_guid[] = {SHARED_PAGE_BASE_GUID};

    set_var_req.variable_name_ptr = (uint16_t*)shared_page_base_name;
    set_var_req.variable_name_size = sizeof(shared_page_base_name);
    set_var_req.vendor_namespace_guid = shared_page_base_guid[0];
    set_var_req.data_size = sizeof(uint64_t);

    const uint64_t addr_of_sdl_in_ddr = SDL_RESERVATION_BASE;
    set_var_req.data = (uint8_t*)&addr_of_sdl_in_ddr;

    set_var_req.attributes.as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    sdl_get_mem_ctx(&mem_ctx);
    variable_service_initialize_ctx(&req_ctx, &mem_ctx);

    // Call variable service to set SDL variable
    BUG_ASSERT(req_ctx.is_initialized);
    variable_service_sync_set_variable(&req_ctx, &set_var_req);
    variable_service_unlock_get_var_ctx(&req_ctx);
}