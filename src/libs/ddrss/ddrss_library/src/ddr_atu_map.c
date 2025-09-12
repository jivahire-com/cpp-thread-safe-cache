//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_atu_map.c
 * Source file to implement DDR ATU mapping.
 */

/*------------- Includes -----------------*/
#include <atu_lib.h>
#include <bug_check.h>
#include <ddrss.h>
#include <ddrss_lib.h>
#include <kng_soc_constants.h>
#include <silibs_ap_top_regs.h>
#include <silibs_platform.h>
#include <stdint.h>
#include <stdio.h> // for printf
#include <stdlib.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/
#define NUM_DIE 2

#ifndef _8KB
    #define _8KB (0x2000UL)
#endif

/*-------------- Typedefs ----------------*/
static uint32_t ddrss_atu_map_flags[NUM_DIE];
static atu_map_entry_t ddrss_cfg_space_mem_atu_map_struct[NUM_DIE];
static atu_map_entry_t ddr_media_atu_struct = {0};
static atu_entry_attr_t ddr_atu_attr_priv_root = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT};
static bool new_atu_mapping = false;
static atu_map_entry_t atu_map_entry_ns[NUM_DIE];
static atu_map_entry_t atu_map_entry_rt[NUM_DIE];

/*--------- Function Prototypes ----------*/

/*---------------- Function ---------------*/
uintptr_t ddrss_atu_map_cfg_space(uint32_t die_num)
{
    ddrss_atu_map_flags[die_num] = 0;
    int sts = 0;

    /* Creating an ATU map table to map a portion of SCP remappable space into  memory region of AP */
    ddrss_cfg_space_mem_atu_map_struct[die_num].ap_base_address =
        (die_num == 0) ? AP_TOP_D0_DDRSS_0_ADDRESS : AP_TOP_D1_DDRSS_0_ADDRESS;
    ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_start_address = 0;
    ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_end_address =
        (AP_TOP_D0_DDRSS_1_ADDRESS - AP_TOP_D0_DDRSS_0_ADDRESS) * (DDRSS_MAX_SS_NUM / 2) - 1;
    ddrss_cfg_space_mem_atu_map_struct[die_num].attribute.as_uint32 = ddr_atu_attr_priv_root.as_uint32;

    atu_map_entry_t ddr_atu_tmp = ddrss_cfg_space_mem_atu_map_struct[die_num];
    if (atu_find_map(ATU_ID_MSCP, &ddr_atu_tmp) == SILIBS_SUCCESS)
    {
        uint32_t offset = ddr_atu_tmp.ap_base_address - ddrss_cfg_space_mem_atu_map_struct[die_num].ap_base_address;
        ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_start_address = offset + ddr_atu_tmp.mscp_start_address;
        ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_end_address +=
            ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_start_address;
        BUG_ASSERT_PARAM(ddr_atu_tmp.mscp_end_address >= ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_end_address,
                         ddr_atu_tmp.mscp_end_address,
                         ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_end_address);
    }
    else
    {
        sts = atu_map(ATU_ID_MSCP, &ddrss_cfg_space_mem_atu_map_struct[die_num]);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
        ddrss_atu_map_flags[die_num] |= BIT1;
        printf("ATU region mapped to Die 0x%lx 0x%lX\n",
               (unsigned long)die_num,
               (unsigned long)ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_start_address);
    }

    info_print("DDR mem  base: %08x\n", ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_start_address);

    return ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_start_address;
}

void ddrss_atu_unmap_cfg_space(uint32_t die_num)
{
    if (ddrss_atu_map_flags[die_num] & BIT1)
    {
        printf("ATU region unmapped from Die 0x%lx 0x%lX\n",
               (unsigned long)die_num,
               (unsigned long)ddrss_cfg_space_mem_atu_map_struct[die_num].mscp_start_address);
        int sts = atu_unmap(ATU_ID_MSCP, &ddrss_cfg_space_mem_atu_map_struct[die_num]);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
        ddrss_atu_map_flags[die_num] &= ~BIT1;
    }
}

/* Not thread safe.  Assumes ATU map, mapped memory usage, ATU unmap without context switching to another
   thread that may call this function
*/
uintptr_t ddrss_atu_map_media_addr(uint64_t p_addr_8K_aligned)
{
    BUG_ASSERT_PARAM(p_addr_8K_aligned % _8KB == 0, p_addr_8K_aligned, _8KB);

    /* Creating an ATU map table to map a portion of SCP remappable space into memory region of AP */
    ddr_media_atu_struct.ap_base_address = p_addr_8K_aligned;
    ddr_media_atu_struct.mscp_start_address = 0;
    ddr_media_atu_struct.mscp_end_address = _8KB - 1;
    ddr_media_atu_struct.attribute.as_uint32 = ddr_atu_attr_priv_root.as_uint32;

    int retval = atu_map(ATU_ID_MSCP, &ddr_media_atu_struct);
    if (retval != SILIBS_SUCCESS)
    {
        // BUG_ASSERT_PARAM(retval == SILIBS_SUCCESS, retval, 0);
        printf("ERR: atu_map() returned %d\n", retval);
    }
    else
    {
        new_atu_mapping = true;
        printf("ATU region mapped to 0x%lX\n", (unsigned long)ddr_media_atu_struct.mscp_start_address);
    }

    return (ddr_media_atu_struct.mscp_start_address);
}

void ddrss_atu_unmap_media_addr(uint64_t p_addr_8K_aligned)
{
    BUG_ASSERT_PARAM(p_addr_8K_aligned == ddr_media_atu_struct.ap_base_address,
                     p_addr_8K_aligned,
                     ddr_media_atu_struct.ap_base_address);
    if (!new_atu_mapping)
    {
        printf("No need to unmap ATU region since it was already mapped\n");
        return;
    }

    int retval = atu_unmap(ATU_ID_MSCP, &ddr_media_atu_struct);
    if (retval != SILIBS_SUCCESS)
    {
        // BUG_ASSERT_PARAM(retval == SILIBS_SUCCESS, retval, 0);
        printf("ERR: atu_unmap() returned %d\n", retval);
    }
    else
    {
        printf("ATU region unmapped\n");
    }
}

uintptr_t ddrss_atu_map_fips_ns_space(uint32_t die_num)
{
    atu_entry_attr_t fips_test_atu_ns_attr = {ATU_BUS_ATTR_NS};
    atu_map_entry_ns[die_num].ap_base_address =
        (die_num == SOC_D0) ? DDR_MEMORY_NUMA_DOMAIN0_START : DDR_MEMORY_NUMA_DOMAIN1_START;
    atu_map_entry_ns[die_num].mscp_start_address = 0;
    atu_map_entry_ns[die_num].mscp_end_address = 64 * SL_1KB - 1;
    atu_map_entry_ns[die_num].attribute = fips_test_atu_ns_attr;
    int sts = atu_map(ATU_ID_MSCP, &atu_map_entry_ns[die_num]);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
    printf("ATU region mapped to Die 0x%lx ns_mem:0x%lX\n",
           (unsigned long)die_num,
           (unsigned long)atu_map_entry_ns[die_num].mscp_start_address);
    return atu_map_entry_ns[die_num].mscp_start_address;
}

uintptr_t ddrss_atu_map_fips_rt_space(uint32_t die_num)
{
    atu_entry_attr_t fips_test_atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT};
    atu_map_entry_rt[die_num].ap_base_address =
        (die_num == SOC_D0) ? DDR_MEMORY_NUMA_DOMAIN0_START : DDR_MEMORY_NUMA_DOMAIN1_START;
    atu_map_entry_rt[die_num].mscp_start_address = 0;
    atu_map_entry_rt[die_num].mscp_end_address = 64 * SL_1KB - 1;
    atu_map_entry_rt[die_num].attribute = fips_test_atu_root_attr;
    int sts = atu_map(ATU_ID_MSCP, &atu_map_entry_rt[die_num]);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
    printf("ATU region mapped to Die 0x%lx rt_mem:0x%lX\n",
           (unsigned long)die_num,
           (unsigned long)atu_map_entry_rt[die_num].mscp_start_address);
    return atu_map_entry_rt[die_num].mscp_start_address;
}

void ddrss_atu_unmap_fips_space(uint32_t die_num)
{
    int sts;

    sts = atu_unmap(ATU_ID_MSCP, &atu_map_entry_rt[die_num]);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
    sts = atu_unmap(ATU_ID_MSCP, &atu_map_entry_ns[die_num]);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
}
