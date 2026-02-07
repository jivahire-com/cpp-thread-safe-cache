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

#include <DfwkClient.h>
#include <FpFwUtils.h>
#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <atu_api.h>
#include <atu_lib.h>
#include <bug_check.h>
#include <cmsis_m7.h>
#include <ddrss_sdl.h>
#include <fpfw_cfg_mgr.h>
#include <idhw.h>
#include <idsw.h>
#include <kng_scp_tfa_shared.h>
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
// #ifndef PPR_DBG
//     #define PPR_DBG 1
// #endif

#if PPR_DBG
    #define PPR_DBG(...) FPFW_DBGPRINT_INFO(__VA_ARGS__)
#else
    #define PPR_DBG(...) \
        do               \
        {                \
        } while (0)
#endif
/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void store_sdl_var_complete(void* context, var_service_req_ctx_t* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size);

/*-- Declarations (Statics and globals) --*/
static atu_map_entry_t g_sdl_mem_atu_map_struct = {0};

// Keep static for async operations - must persist until callback completes
static var_service_shared_mem_t g_sdl_mem_ctx = {0};
static var_service_req_ctx_t g_sdl_req_ctx = {0};
static var_service_req_params_t g_sdl_set_var_req = {0};

/*------------- Functions ----------------*/

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
        PPR_DBG("SDL ATU map not initialized\n");
        BUG_ASSERT(g_sdl_mem_atu_map_struct.mscp_start_address != 0);
    }

    return g_sdl_mem_atu_map_struct.mscp_start_address;
}

void sdl_get_mem_ctx(var_service_shared_mem_t* mem_ctx)
{
    // SCP_EXP_SDL_LOAD_BASE is an overlapping region with PCIe PHY FW load region for SDL variable service
    // SDL only needs it during early boot to load from flash to DDR reserved region
    mem_ctx->payload_base = (uintptr_t)SCP_EXP_SDL_LOAD_BASE;
    mem_ctx->max_payload_size = SCP_EXP_SDL_LOAD_SIZE;
}

uintptr_t get_sdl_arsm0_addr(void)
{
    return (uintptr_t)MSCP_ATU_AP_WINDOW_ARSM_DIE_0_BASE_ADDR + ARSM_GET_REGION_OFFSET(D0_ARSM_SDL_RESERVED_BASE);
}

// ============================================================================
// SDL Header Utilities
// ============================================================================

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
        (uint32_t)CalculateRemoteCheckSum16((uint32_t)&empty_sdl_header, sizeof(empty_sdl_header));

    memcpy((void*)(uint32_t)dest_addr, (void*)&empty_sdl_header, sizeof(empty_sdl_header));
}

/**
 * @brief Retrieves the total size of the SDL from its header.
 *
 * Reads the SDL header at the given base address and calculates the total
 * size by:
 *     Header length + (DefectCount * sizeof(MEMORY_DEFECT_V2))
 *
 * @param sdl_base Base address of the SDL in memory
 * @return Total size of the SDL in bytes
 */
size_t sdl_calculate_size_by_defect_count(uintptr_t sdl_base)
{
    volatile PMEMORY_DEFECT_LIST_HEADER sdl_header = (volatile PMEMORY_DEFECT_LIST_HEADER)(uint32_t)sdl_base;
    return (size_t)(sizeof(MEMORY_DEFECT_LIST_HEADER)) + (size_t)(sdl_header->DefectCount * sizeof(MEMORY_DEFECT_V2));
}

size_t sdl_get_size_from_header(uintptr_t sdl_base)
{
    volatile PMEMORY_DEFECT_LIST_HEADER sdl_header = (volatile PMEMORY_DEFECT_LIST_HEADER)(uint32_t)sdl_base;
    return (size_t)(sdl_header->Length);
}

bool sdl_signature_is_valid(uintptr_t sdl_base)
{
    volatile PMEMORY_DEFECT_LIST_HEADER sdl_header = (volatile PMEMORY_DEFECT_LIST_HEADER)(uint32_t)sdl_base;
    if (sdl_header->Signature != PSHED_PI_DEFECT_LIST_SIGNATURE)
    {
        PPR_DBG("SDL signature invalid: 0x%X\n", (unsigned int)sdl_header->Signature);
        return false;
    }

    return true;
}

void sdl_update_checksum(uintptr_t sdl_base)
{
    volatile PMEMORY_DEFECT_LIST_HEADER sdl_header = (volatile PMEMORY_DEFECT_LIST_HEADER)(uint32_t)sdl_base;
    sdl_header->Checksum = 0;
    uint16_t checksum = CalculateRemoteCheckSum16((uint32_t)sdl_header, sdl_calculate_size_by_defect_count(sdl_base));
    PPR_DBG("Updated SDL checksum to 0x%X\n", checksum);
    sdl_header->Checksum = (uint32_t)checksum;
    __DSB();
}

static void sdl_init_var_ctx(var_service_shared_mem_t* mem_ctx, var_service_req_ctx_t* req_ctx)
{
    sdl_get_mem_ctx(mem_ctx);
    memset((void*)mem_ctx->payload_base, 0, mem_ctx->max_payload_size);
    variable_service_initialize_ctx(req_ctx, mem_ctx);
}

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

// ============================================================================
// SDL Flash Operations (Load)
// ============================================================================

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
    status = variable_service_sync_get_variable(&req_ctx, &req_params);
    PPR_DBG("SDL sync. get var sts: 0x%lx\n", (unsigned long)status);
    variable_service_unlock_get_var_ctx(&req_ctx);

    return status;
}

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

// ============================================================================
// SDL Flash Operations (Store - Async)
// ============================================================================

void store_sdl_var_complete(void* context, var_service_req_ctx_t* var_serv_ctx, uint8_t* data_start_ptr, size_t data_size)
{
    FPFW_UNUSED(data_start_ptr);
    FPFW_UNUSED(data_size);

    // Context is the PDFWK_ASYNC_REQUEST_HEADER
    PDFWK_ASYNC_REQUEST_HEADER request = (PDFWK_ASYNC_REQUEST_HEADER)context;

    int status = (int)var_serv_ctx->async_req_result;
    if (status != KNG_SUCCESS)
    {
        PPR_DBG("SDL store to flash failed with status 0x%x\n", status);
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_SDL_STORE_FAILED, status);
        BUG_ASSERT_PARAM(false, status, 0);
    }
    else
    {
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_STORED_TO_FLASH);
    }

    sdl_unmap_atu(); // Unmap the ATU mapping for SDL

    // Complete the async operation request
    if (request != NULL)
    {
        PPR_DBG("SDL store complete\n");
        DfwkAsyncRequestComplete(request);
    }
    else
    {
        PPR_DBG("No DFWK request context provided for SDL store completion.\n");
    }
}

// This is used at shutdown to write the SDL from DDR back to flash as a UEFI var.
void store_sdl_var_async(void* ssi_request)
{
    if (idsw_get_die_id() == DIE_1)
    {
        return;
    }

    uint16_t sdl_var_name[] = SDL_VAR_NAME;
    const guid_t sdl_var_guid = SDL_VAR_GUID;

    sdl_init_var_ctx(&g_sdl_mem_ctx, &g_sdl_req_ctx);

    // Map SDL DDR region and get ATU start address
    // Unmap ATU in the callback
    sdl_map_atu(SDL_RESERVATION_BASE);
    g_sdl_set_var_req.data = (uint8_t*)sdl_get_atu_start_addr(); // <--- This must be the ATU mapped address
    sdl_fill_var_params(&g_sdl_set_var_req,
                        (uint16_t*)sdl_var_name,
                        sizeof(sdl_var_name),
                        &sdl_var_guid,
                        g_sdl_set_var_req.data,
                        SDL_MAX_SIZE);

    BUG_ASSERT(g_sdl_req_ctx.is_initialized);
    int status = variable_service_async_set_variable(&g_sdl_req_ctx, &g_sdl_set_var_req, store_sdl_var_complete, ssi_request);

    if (status != KNG_SUCCESS)
    {
        DDR_MANAGER_ET_ERROR(DDR_MANAGER_ET_TYPE_SDL_STORE_FAILED, status);
        BUG_ASSERT_PARAM(false, status, 0);
    }
}

// ============================================================================
// SDL Flash Operations (Store - Sync)
// ============================================================================
void store_sdl_var_sync_from_arsm0(void)
{
    if (idsw_get_die_id() == DIE_1)
    {
        return;
    }

    uint16_t sdl_var_name[] = SDL_VAR_NAME;
    const guid_t sdl_var_guid = SDL_VAR_GUID;

    sdl_init_var_ctx(&g_sdl_mem_ctx, &g_sdl_req_ctx);

    // Find ATU start address for SDL in ARSM0
    g_sdl_set_var_req.data = (uint8_t*)get_sdl_arsm0_addr();
    sdl_fill_var_params(&g_sdl_set_var_req,
                        (uint16_t*)sdl_var_name,
                        sizeof(sdl_var_name),
                        &sdl_var_guid,
                        g_sdl_set_var_req.data,
                        SDL_MAX_SIZE);

    BUG_ASSERT(g_sdl_req_ctx.is_initialized);
    variable_service_sync_set_variable(&g_sdl_req_ctx, &g_sdl_set_var_req);
    variable_service_unlock_get_var_ctx(&g_sdl_req_ctx);
}

// ============================================================================
// SDL Address Publishing
// ============================================================================

void ddr_publish_sdl_addr(void)
{
    if (idsw_get_die_id() == DIE_1)
    {
        return;
    }

    if (config_get_skip_sdl_address_publishing())
    {
        PPR_DBG("Skipping SDL address publishing as per config knob\n");
        DDR_MANAGER_ET_STATUS(DDR_MANAGER_ET_TYPE_SDL_ADDRESS_PUBLISHING_SKIPPED);
        return;
    }

    PPR_DBG("Publishing SDL DDR location to UEFI variable\n");

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
// SDL Entry Update (PPR Completion)
// ============================================================================

// SDL is currently in ARSM0 following PPR completion
// Attempts to match a ddrss_addr_t defect entry to an SDL defect entry and update PPR completion status

// Todo: Next PR + add tests