//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_manager_sdl.c
 * This file contains APIs relating to Memory SDL (Shared Defect List) APIs
 */

/*------------- Includes -----------------*/
#include "ddr_manager.h"
#include "ddr_manager_events.h"
#include "ddr_manager_i.h"
#include "ddr_memory_map.h"

#include <FpFwUtils.h>
#include <arm_intrinsic.h>
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <cmsis_m7.h>
#include <ddrss_sdl.h>
#include <fpfw_cfg_mgr.h>
#include <idhw.h>
#include <idsw.h>
#include <mscp_exp_rmss_memory_map.h>
#include <stdio.h>
#include <variable_services.h>

// Some host builds (e.g., Windows unit tests) lack CMSIS __ISB; provide a harmless fallback there.
#ifdef _WIN32
    #ifndef __ISB
        #define __ISB() ((void)0)
    #endif
#endif

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
static void sdl_fill_var_params(var_service_req_params_t* req_params,
                                uint16_t* var_name,
                                size_t var_name_size_bytes,
                                const guid_t* var_guid,
                                uint8_t* data_ptr,
                                uint32_t data_size);

/*-- Declarations (Statics and globals) --*/
static atu_map_entry_t g_sdl_mem_atu_map_struct = {0};

/*------------- Functions ----------------*/

// ============================================================================
// SDL Flash Operations (Load)
// ============================================================================
void load_shared_defect_list_to_DDR(void)
{
    if (idsw_get_die_id() == DIE_1)
    {
        return;
    }

    DDR_LOG_INFO("Load SDL->DDR\n");

    sdl_map_atu(SDL_RESERVATION_BASE); // This is the reserved DDR region for SDL
    int32_t status = load_sdl_from_flash(sdl_get_atu_start_addr());
    if (status == KNG_E_NOT_FOUND)
    {
        // Create an empty SDL Header and copy to reserved DDR region
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_NOT_FOUND);
        copy_empty_sdl_header_to_reserved_ddr(sdl_get_atu_start_addr());
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_CREATED_EMPTY_SDL_HEADER);
    }
    else if (status == KNG_SUCCESS)
    {
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_FOUND_AND_LOADED_TO_DDR);
    }
    else
    {
        // Some other error occurred
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_SDL_COPY_TO_DDR_FAILED, status);
        BUG_ASSERT_PARAM(false, status, 0);
    }

    sdl_unmap_atu();
}

int32_t load_sdl_from_flash(uintptr_t load_addr)
{
    int32_t status = KNG_SUCCESS;
    var_service_shared_mem_t mem_ctx = {0};
    var_service_req_ctx_t req_ctx = {0};
    var_service_req_params_t req_params = {0};
    uint16_t sdl_var_name[] = SDL_VAR_NAME;
    const guid_t sdl_var_guid = SDL_VAR_GUID;

    sdl_get_mem_ctx(&mem_ctx);
    memset((void*)mem_ctx.payload_base, 0, mem_ctx.max_payload_size);

    // Initialize variable service context
    variable_service_initialize_ctx(&req_ctx, &mem_ctx);

    // Prepare GetVariable request parameters
    req_params.attributes_size = 0;
    sdl_fill_var_params(&req_params, (uint16_t*)sdl_var_name, sizeof(sdl_var_name), &sdl_var_guid, (uint8_t*)load_addr, SDL_VAR_SIZE_BYTES);

    // Call variable service to get SDL variable
    status = variable_service_sync_get_variable(&req_ctx, &req_params); // This calls its own variable_service_unlock_get_var_ctx
    DDR_LOG_INFO("SDL sync. get var sts: 0x%lx\n", (unsigned long)status);

    return status;
}

// =============================================================
// Helpers
// =============================================================
static void sdl_fill_var_params(var_service_req_params_t* req_params,
                                uint16_t* var_name,
                                size_t var_name_size_bytes,
                                const guid_t* var_guid,
                                uint8_t* data_ptr,
                                uint32_t data_size)
{
    req_params->variable_name_ptr = var_name;
    req_params->variable_name_size = (uint32_t)var_name_size_bytes;
    memcpy(&req_params->vendor_namespace_guid, var_guid, sizeof(*var_guid));
    req_params->data = data_ptr;
    req_params->data_size = data_size;
    req_params->attributes_size = 0;
    req_params->attributes.as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;
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

    memset((void*)(uint32_t)dest_addr, 0, sizeof(empty_sdl_header));
    memcpy((void*)(uint32_t)dest_addr, (void*)&empty_sdl_header, sizeof(empty_sdl_header));
}

// Writes a UEFI variable as a hand-off to OS with the address of the SDL region in DDR
void ddr_publish_sdl_addr(void)
{
    if (idsw_get_die_id() == DIE_1)
    {
        return;
    }

    if (config_get_skip_sdl_address_publishing())
    {
        DDR_LOG_INFO("Skipping SDL address publishing as per config knob\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_ADDRESS_PUBLISHING_SKIPPED);
        return;
    }

    DDR_LOG_INFO("Publishing SDL DDR location to UEFI variable\n");

    var_service_req_ctx_t req_ctx = {0};
    var_service_shared_mem_t mem_ctx = {0};
    var_service_req_params_t set_var_req = {0};
    uint16_t shared_page_base_name[] = SHARED_PAGE_BASE_NAME;
    const guid_t shared_page_base_guid = SHARED_PAGE_BASE_GUID;

    set_var_req.variable_name_ptr = (uint16_t*)shared_page_base_name;
    set_var_req.variable_name_size = sizeof(shared_page_base_name);
    set_var_req.vendor_namespace_guid = shared_page_base_guid;
    set_var_req.data_size = sizeof(uint64_t);

    const uint64_t addr_of_sdl_in_ddr = SDL_RESERVATION_BASE;
    set_var_req.data = (uint8_t*)&addr_of_sdl_in_ddr;

    set_var_req.attributes.as_uint32 = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

    sdl_get_mem_ctx(&mem_ctx);
    variable_service_initialize_ctx(&req_ctx, &mem_ctx);

    BUG_ASSERT(req_ctx.is_initialized);
    variable_service_sync_set_variable(&req_ctx, &set_var_req);
    variable_service_unlock_get_var_ctx(&req_ctx);
}

void cleanup_after_sdl_load(void)
{
    // Align to cache-line boundaries so cache maintenance touches whole lines and cannot leave stale bytes when other data shares a line.
    const uintptr_t cache_line = 32; // Cortex-M7 D-cache line size in bytes
    uintptr_t base = (uintptr_t)SCP_EXP_SDL_LOAD_BASE;
    uintptr_t end = base + (uintptr_t)SCP_EXP_SDL_LOAD_SIZE;
    uintptr_t aligned_base = base & ~(cache_line - 1U);
    uintptr_t aligned_end = (end + cache_line - 1U) & ~(cache_line - 1U);
    size_t aligned_size = (size_t)(aligned_end - aligned_base);

    // Zero the staging window before reuse; this region overlaps with other early-boot payloads.
    memset((void*)SCP_EXP_SDL_LOAD_BASE, 0, SCP_EXP_SDL_LOAD_SIZE);

    // Clean+invalidate the aligned span so the zero fill is committed to DDR and the local cache stops serving old data.
    SCB_CleanInvalidateDCache_by_Addr((uint32_t*)aligned_base, (int32_t)aligned_size);
    __DSB();
    __ISB();
}

// ============================================================================
// ATU Mapping & Address Functions
// ============================================================================
void sdl_map_atu(uint64_t base_addr)
{
    atu_entry_attr_t sdl_atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_NS};

    // This is the ultimate destination for where the variable should end up
    g_sdl_mem_atu_map_struct.ap_base_address = base_addr;
    g_sdl_mem_atu_map_struct.mscp_start_address = 0;
    g_sdl_mem_atu_map_struct.mscp_end_address = ALIGN_UP(SDL_RESERVATION_SIZE, ATU_PAGE_SIZE) - 1;
    g_sdl_mem_atu_map_struct.attribute.as_uint32 = sdl_atu_root_attr.as_uint32;

    int sts = atu_map(ATU_ID_MSCP, &g_sdl_mem_atu_map_struct);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
}

void sdl_unmap_atu(void)
{
    BUG_ASSERT(g_sdl_mem_atu_map_struct.ap_base_address != 0);
    int sts = atu_unmap(ATU_ID_MSCP, &g_sdl_mem_atu_map_struct);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
}

uintptr_t sdl_get_atu_start_addr(void)
{
    if (g_sdl_mem_atu_map_struct.mscp_start_address == 0)
    {
        DDR_LOG_INFO("SDL ATU map not initialized\n");
        BUG_ASSERT(g_sdl_mem_atu_map_struct.mscp_start_address != 0);
    }

    return g_sdl_mem_atu_map_struct.mscp_start_address;
}