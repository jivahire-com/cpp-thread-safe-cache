//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @ddr_memory_map.c
 * This file contains the implementation for creating a memory map that will be passed to UEFI.
 * The initial memory map is requested from Silicon Libs, reserved regions are added, and the map is
 * reformatted.
 */

/*------------- Includes -----------------*/
#include "ddr_manager_i.h"
#include "ddr_memory_map.h"
#include "memory_map/ddrss_reserved_regions.h"
#include "sds_api.h"
#include "sds_configuration.h"
#include "shared_sds_def.h"

#include <ErrorHandler.h> // for FPFwErrorRaise
#include <stdio.h>

/*-- Symbolic Constant Macros (defines) --*/
// This macro is defined in silibs/soc_fw_shared/include/memory_map/ddrss_reserved_regions.h.
GENERATE_ARRAY_OF_RSVD_REGIONS

#define Memory_MODULE_NAME "SDS_Memory_Region"

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
void insert_range(ddrss_memory_region_t mmap[], int idx, uint64_t start, uint64_t end, int reserved_for_ap, int pas_mask);
void show_map(const ddrss_memory_region_t this_mmap[], int idx, bool show_all);
static uint32_t MemoryMapPassToTFA(ddrss_memory_region_t mmap_tfa[]);

/*-- Declarations (Statics and globals) --*/
ddrss_memory_region_t appended_mmap_tfa[MAX_MEMORY_REGIONS] = {0};

/*------------- Functions ----------------*/
/**
 *  Create DDR Address map with appended M/SCP reservations and pass to TF-A through SDS structure.
 *    If supported, SCP will get a DDR address regions map from ddrss si. lib
 *    Otherwise, SCP will pass a predefined map - if the build target/platform is supported
 *
 *  @param
 *      none
 *
 *  @return
 *      none
 */
void ddr_create_memory_map()
{
    const ddrss_sys_mem_region_t* all_mem_regions = NULL;
    ddrss_memory_region_t sorted_reservations[ARRAY_OF_RSVD_REGIONS_COUNT] = {};
    int status;

    printf("Creating DDR Memory Map\n");

    // Update ptr to memory info table from ddrss library or from static table defined in ddr_memory_map.h
    ddrss_get_memory_map(&all_mem_regions);

    // Check & sort the reserved region array into ascending order before splicing with the incoming memory map.
    sort_reserved_regions(_dram_rsvd_regions, ARRAY_OF_RSVD_REGIONS_COUNT, sorted_reservations);

    status = check_reservation_order(sorted_reservations);
    if (status != SILIBS_SUCCESS)
    {
        FPFwErrorRaise(status, 0, 0, 0, 0);
    }
    // Add reserved regions
    // Walk the memory regions and insert reserved ranges.  Returns populated memory map as 3rd parameter
    if (ddrmap_add_reservations((*all_mem_regions).mem_regions, sorted_reservations, appended_mmap_tfa) != SILIBS_SUCCESS)
    {
        printf("[DDR] Error adding reserved range to DDR memory map");
    }

    // Pass to TF-A in Shared SRAM via SDS structure service
    if (MemoryMapPassToTFA(appended_mmap_tfa) != DFWK_SUCCESS)
    {
        printf("[DDR] Error using SDS structure");
    }
}

/**
 *  Using SDS api to pass DDR Memory Map to TFA
 *
 *  @param
 *      IN - Pointer to array of 64bit formatted array of memory regions
 *
 *  @return
 *      int DFWK_SUCCESS or DFWK_FAILED
 */
static uint32_t MemoryMapPassToTFA(ddrss_memory_region_t mmap_tfa[])
{
    size_t numbytes_new_mmap = 0;

    sds_block_creation(SDS_MEMORY_MAP_STRUCT_ID, SDS_MEMORY_MAP_SIZE, PLATFORM_SDS_REGION_ARSM_DIE0);

    if (get_mmap_size_bytes(mmap_tfa, &numbytes_new_mmap) != SILIBS_SUCCESS)
    {
        printf("[DDR] Error reading DDR memory map size");
    }

    int result = sds_block_write(SDS_MEMORY_MAP_STRUCT_ID, mmap_tfa, numbytes_new_mmap);

    return result;
}

/**
 * @brief
 *  Retrieves base DDR Memory Map from either ddrss silicon library
 *  This map should not include any SCP or MCP address reservations - that is handled elsewhere
 *
 *  @param
 *      Takes a pointer to pointer of const ddrss_sys_mem_region_t - such that the pointer may be modified by this function
 *
 *  @return
 *      none
 */
void ddrss_get_memory_map(const ddrss_sys_mem_region_t** all_mem_regions)
{
    *all_mem_regions = ddrss_get_system_mem_region();
    printf("[DDR] Retrieving DDR Memory Info from library\n");

    printf("[DDR] Number of memory regions before adding reservations = %lu\n",
           (unsigned long)(*all_mem_regions)->num_regions);
}

/**
 *  Sort the reserved region array into ascending order before splicing with the incoming memory map.
 *
 *  @param
 *      IN - Pointer to const array of _dram_rsvd_regions
 *      IN - number of regions
 *      OUT - out_mmap - sorted reserved regions
 *
 *      Incoming and outging arrays are terminated by an all-zero ddrss_memory_region_t
 *
 *  @return
 *      int SILIBS_SUCCESS or SILIBS_E_DATA
 */
int sort_reserved_regions(const ddrss_memory_region_t reservations[], uint32_t num_rsvd, ddrss_memory_region_t out_sorted[])
{
    uint32_t rsvd_count = 0;
    int pre_idx = 0;
    ddrss_memory_region_t curr_region[ARRAY_OF_RSVD_REGIONS_COUNT] = {};

    for (size_t i = 0; i < num_rsvd; i++)
    {
        out_sorted[i] = reservations[i];
    }
    printf("[DDR] --------original memory map----------- \n");
    show_map(out_sorted, ddrmap_get_last_idx(out_sorted), true);

    // insertion sort
    for (rsvd_count = 1; rsvd_count < num_rsvd; rsvd_count++)
    {
        // Check for exit condition
        if (out_sorted[rsvd_count].start_address == 0 && out_sorted[rsvd_count].end_address == 0)
        {
            printf("[DDR] --------sorted memory map----------- \n");
            show_map(out_sorted, ddrmap_get_last_idx(out_sorted), true);
            return SILIBS_SUCCESS;
        }

        curr_region[rsvd_count] = out_sorted[rsvd_count];
        pre_idx = rsvd_count - 1;

        while (pre_idx >= 0 && out_sorted[pre_idx].start_address > curr_region[rsvd_count].start_address)
        {
            out_sorted[pre_idx + 1] = out_sorted[pre_idx];
            pre_idx--;
        }
        out_sorted[pre_idx + 1] = curr_region[rsvd_count];
    }

    printf("[DDR] Error while sorting reserved memory map");
    return SILIBS_E_DATA;
}

/**
 *  Check the reservations are in ascending order & not overlapping
 *
 *  @param
 *      IN - sorted reserved regions
 *
 *  @return
 *      int SILIBS_SUCCESS or SILIBS_E_DATA
 */
int check_reservation_order(const ddrss_memory_region_t reservations[])
{
    uint32_t idx = 1;
    uint64_t curr_start;
    uint64_t prev_end;

    while (idx < ARRAY_OF_RSVD_REGIONS_COUNT)
    {
        curr_start = reservations[idx].start_address;
        prev_end = reservations[idx - 1].end_address;

        if (curr_start == 0)
        {
            return SILIBS_SUCCESS;
        }

        if (!(curr_start >= prev_end))
        {
            printf("[DDR] Unexpected reservation overlap (0x%llx)\n", curr_start);
            return SILIBS_E_DATA;
        }
        idx++;
    }
    return SILIBS_SUCCESS;
}

/**
 *  Add SCP/MCP DDR address reservations to DDR Memory Map
 *  The 'PAS' flag is overwritten when adding a reserved range.
 *
 *  @param
 *      IN - Pointer to array of 64bit formatted array of memory regions
 *      IN - Pointer to const array of _dram_rsvd_regions
 *      OUT - out_mmap - Incoming memory regions + added reserved regions
 *
 *      Incoming and outging arrays are terminated by an all-zero ddrss_memory_region_t
 *
 *  @return
 *      int SILIBS_SUCCESS or SILIBS_E_DATA
 */
int ddrmap_add_reservations(const ddrss_memory_region_t in_mmap[],
                            const ddrss_memory_region_t reservations[],
                            ddrss_memory_region_t out_mmap[])
{
    uint32_t in_idx = 0;
    uint32_t out_idx = 0;
    uint32_t res_idx = 0;
    uint64_t next_start = 0;

    printf("[DDR] --------Incoming memory map------------- \n");
    show_map(in_mmap, ddrmap_get_last_idx(in_mmap), true);
    printf("[DDR] --------DDR Reservations---------------- \n");
    show_map((ddrss_memory_region_t*)reservations, ddrmap_get_last_idx(reservations), true);
    printf("[DDR] --------Outgoing memory map------------- \n");

    while (in_idx < MAX_MEMORY_REGIONS)
    {
        // Check for exit condition
        if (in_mmap[in_idx].start_address == 0 && in_mmap[in_idx].end_address == 0)
        {
            // Append last memory region of all 0s
            out_mmap[out_idx] = in_mmap[in_idx];
            return SILIBS_SUCCESS;
        }

        next_start = in_mmap[in_idx].start_address;
        while (next_start < in_mmap[in_idx].end_address)
        {
            // Insert reserved range
            if (next_start == reservations[res_idx].start_address)
            {
                next_start = MIN(reservations[res_idx].end_address, in_mmap[in_idx].end_address);

                // set PAS per reserved regions requirement
                int pas_mask = reservations[res_idx].attr.as_uint32;
                insert_range(out_mmap,
                             out_idx,
                             reservations[res_idx].start_address,
                             next_start,
                             reservations[res_idx].attr.available_sysmem,
                             pas_mask);
                show_map(out_mmap, out_idx, false);
                out_idx++;
                res_idx++;
            }

            // Insert portion of existing range
            else if (next_start < reservations[res_idx].start_address)
            {
                insert_range(out_mmap,
                             out_idx,
                             next_start,
                             MIN(in_mmap[in_idx].end_address, reservations[res_idx].start_address),
                             in_mmap[in_idx].attr.available_sysmem,
                             in_mmap[in_idx].attr.as_uint32);
                next_start = reservations[res_idx].start_address == 0
                                 ? in_mmap[in_idx].end_address
                                 : MIN(in_mmap[in_idx].end_address, reservations[res_idx].start_address);
                show_map(out_mmap, out_idx, false);
                out_idx++;
            }

            // Catch corner case where reservation begins out of bounds
            else if ((next_start > reservations[res_idx].start_address) && (reservations[res_idx].start_address != 0))
            {
                insert_range(out_mmap,
                             out_idx,
                             next_start,
                             MIN(in_mmap[in_idx].end_address, reservations[res_idx].end_address),
                             reservations[res_idx].attr.available_sysmem,
                             in_mmap[in_idx].attr.as_uint32);
                next_start = MIN(in_mmap[in_idx].end_address, reservations[res_idx].end_address);
                show_map(out_mmap, out_idx, false);
                out_idx++;
                res_idx++;
            }

            // Insert portion of incoming range
            else if (next_start < in_mmap[in_idx].end_address)
            {
                // Here is a corner case where next reservation could be 0 to signify the end of the list
                insert_range(out_mmap,
                             out_idx,
                             next_start,
                             in_mmap[in_idx].end_address,
                             in_mmap[in_idx].attr.available_sysmem,
                             in_mmap[in_idx].attr.as_uint32);
                next_start = reservations[res_idx].start_address == 0
                                 ? in_mmap[in_idx].end_address
                                 : MIN(in_mmap[in_idx].end_address, reservations[res_idx].start_address);
                show_map(out_mmap, out_idx, false);
                out_idx++;
            }
        }
        in_idx++;
    }
    printf("[DDR] Error while inserting reserved memory ranges for MSCP");
    return SILIBS_E_DATA;
}

/**
 *  Simple helper function to insert an element into memory map array
 *
 *  @param
 *      IN  - Pointer to array of ddrss_memory_region_t
 *      IN  - Array index where to insert next record
 *      IN  - starting address
 *      IN  - ending address
 *      IN  - attr.available_sysmem
 *      IN  - attr.pas_mask
 */
void insert_range(ddrss_memory_region_t mmap[], int idx, uint64_t start, uint64_t end, int reserved_for_ap, int pas_mask)
{
    mmap[idx].start_address = start;
    mmap[idx].end_address = end;
    mmap[idx].attr.as_uint32 = pas_mask;
    mmap[idx].attr.available_sysmem = reserved_for_ap;
};

/**
 *  Get array index of last non-zero record in a ddrss_memory_region_t array
 *      Helper function for helper functions
 *  @param
 *      IN  - Pointer to array of ddrss_memory_region_t structs
 *            The intention is that this is used as a helper for managing the reserved memory ranges
 *  @return
 *      int  Index of last valid ddrss_memory_region_t in the passed-in memory map
 */
int ddrmap_get_last_idx(const ddrss_memory_region_t ddr_mmap[])
{
    // Find the last element in the array (start == end == 0)
    uint32_t mem_idx = 0;
    while (mem_idx < MAX_MEMORY_REGIONS)
    {
        if ((ddr_mmap[mem_idx].start_address == 0) && (ddr_mmap[mem_idx].end_address == 0))
        {
            return mem_idx;
        }
        mem_idx++;
    }

    printf("[DDR] Error - Reserved DDR address range is not terminated by an empty record");
    return SILIBS_E_DATA;
}

/**
 *  Get size in bytes of a memory map
 *  @param
 *      IN  - Pointer to array of ddrss_memory_region_t structs
 *      OUT - Pointer to size_t variable - Number of bytes including terminating empty record
 *            Not modified if memory range array not terminated by 0 start/end descriptor
 *
 *  @return
 *      SILIBS_SUCCESS or SILIBS_E_DATA
 */
int get_mmap_size_bytes(ddrss_memory_region_t appended_mmap_64b[], size_t* num_bytes)
{
    int num_regions = 0;
    int mem_region_count = 0;

    for (uint32_t region_idx = 0; region_idx < MAX_MEMORY_REGIONS; region_idx++)
    {
        // We want to include the final region as all 0s to indicate the end of the reserved ranges
        if (appended_mmap_64b[region_idx].start_address == 0 && appended_mmap_64b[region_idx].end_address == 0)
        {
            num_regions = mem_region_count + 1;
            *num_bytes = (num_regions * sizeof(ddrss_memory_region_t));
            return SILIBS_SUCCESS;
        }
        mem_region_count++;
    }

    printf("[DDR] Discovered memory regions exceeds expected size");
    return SILIBS_E_DATA;
}

/**
 *  Simple helper function to display memory map regions
 *
 *  @param
 *      IN  - Pointer to array of memory_range_descriptor
 *      IN  - Single element's index or total size of array to iterate over - depending on value of 'show_all'
 *      IN  - show_all == true  -> iterate over the entire array up to idx
 *            show_all == false -> display single element of the array
 */
void show_map(const ddrss_memory_region_t this_mmap[], int idx, bool show_all)
{
    if (show_all)
    {
        for (int i = 0; i < idx; i++)
        {
            printf("[DDR] Region: %d \n", i);
            printf("[DDR] \tstart: 0x%llX \n", this_mmap[i].start_address);
            printf("[DDR] \tend:   0x%llX \n", this_mmap[i].end_address);
            printf("[DDR] \tflags.available_sysmem:     %d \n", (int)this_mmap[i].attr.available_sysmem);
            printf("[DDR] \tflags.pas_mask:   0x%x \n", (int)this_mmap[i].attr.as_uint32 & 0xf);
        }
    }
    else
    {
        printf("[DDR] Region: %d \n", idx);
        printf("[DDR] \tstart: 0x%llX \n", this_mmap[idx].start_address);
        printf("[DDR] \tend:   0x%llX \n", this_mmap[idx].end_address);
        printf("[DDR] \tflags.available_sysmem:     %d \n", (int)this_mmap[idx].attr.available_sysmem);
        printf("[DDR] \tflags.pas_mask:   0x%x \n", (int)this_mmap[idx].attr.as_uint32 & 0xf);
    }
}