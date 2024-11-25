//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_atu_map.c
 * Source file to implement DDR ATU mapping.
 */

/*------------- Includes -----------------*/
#include <atu_lib.h>
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
#define DDR_ASSERT ASSERT_FAIL
#define NUM_DIE    2

/*-------------- Typedefs ----------------*/
static uint32_t ddrss_atu_map_flags[NUM_DIE];
static atu_map_entry_t ddrss_mem_atu_map_struct[NUM_DIE];
static atu_entry_attr_t ddrss_test_atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT};

/*--------- Function Prototypes ----------*/

/*---------------- Function ---------------*/
uintptr_t ddrss_atu_map(uint32_t die_num)
{
    ddrss_atu_map_flags[die_num] = 0;

    /* Creating an ATU map table to map a portion of SCP remappable space into  memory region of AP */
    ddrss_mem_atu_map_struct[die_num].ap_base_address = (die_num == 0) ? AP_TOP_D0_DDRSS_0_ADDRESS : AP_TOP_D1_DDRSS_0_ADDRESS;
    ddrss_mem_atu_map_struct[die_num].mscp_start_address = 0;
    ddrss_mem_atu_map_struct[die_num].mscp_end_address =
        (AP_TOP_D0_DDRSS_1_ADDRESS - AP_TOP_D0_DDRSS_0_ADDRESS) * (DDRSS_MAX_SS_NUM / 2) - 1;
    ddrss_mem_atu_map_struct[die_num].attribute.as_uint32 = ddrss_test_atu_root_attr.as_uint32;

    atu_map_entry_t ddr_atu_tmp = ddrss_mem_atu_map_struct[die_num];
    if (atu_find_map(ATU_ID_MSCP, &ddr_atu_tmp) == SILIBS_SUCCESS)
    {
        uint32_t offset = ddr_atu_tmp.ap_base_address - ddrss_mem_atu_map_struct[die_num].ap_base_address;
        ddrss_mem_atu_map_struct[die_num].mscp_start_address = offset + ddr_atu_tmp.mscp_start_address;
        ddrss_mem_atu_map_struct[die_num].mscp_end_address += ddrss_mem_atu_map_struct[die_num].mscp_start_address;
        DDR_ASSERT(ddr_atu_tmp.mscp_end_address >= ddrss_mem_atu_map_struct[die_num].mscp_end_address);
    }
    else
    {
        DDR_ASSERT(!atu_map(ATU_ID_MSCP, &ddrss_mem_atu_map_struct[die_num]));
        ddrss_atu_map_flags[die_num] |= BIT1;
    }

    info_print("DDR mem  base: %08x\n", ddrss_mem_atu_map_struct[die_num].mscp_start_address);

    return ddrss_mem_atu_map_struct[die_num].mscp_start_address;
}

void ddrss_atu_unmap(uint32_t die_num)
{
    if (ddrss_atu_map_flags[die_num] & BIT1)
    {
        DDR_ASSERT(!atu_unmap(ATU_ID_MSCP, &ddrss_mem_atu_map_struct[die_num]));
    }
}
