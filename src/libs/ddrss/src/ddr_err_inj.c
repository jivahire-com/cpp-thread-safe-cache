//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_err_inj.c
 * Source file to implement ddr error injection API functions.
 */

/*------------- Includes -----------------*/
#include <atu_lib.h>
#include <ddr_err_inj.h>
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

/*-------------- Typedefs ----------------*/
uint32_t ddrss_atu_map_flags;
atu_map_entry_t ddrss_mem_atu_map_struct;
atu_entry_attr_t ddrss_test_atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT};

/*--------- Function Prototypes ----------*/

/*---------------- Function ---------------*/
void ddrss_err_inj_atu_map(uint32_t die_num)
{
    uint32_t offset;
    atu_map_entry_t ddr_atu_tmp;
    ddrss_atu_map_flags = 0;

    /* Creating an ATU map table to map a portion of SCP remappable space into  memory region of AP */
    ddrss_mem_atu_map_struct.ap_base_address = (die_num == 0) ? AP_TOP_D0_DDRSS_0_ADDRESS : AP_TOP_D1_DDRSS_0_ADDRESS;
    ddrss_mem_atu_map_struct.mscp_start_address = 0;
    ddrss_mem_atu_map_struct.mscp_end_address =
        (AP_TOP_D0_DDRSS_1_ADDRESS - AP_TOP_D0_DDRSS_0_ADDRESS) * (DDRSS_MAX_SS_NUM / 2) - 1;
    ddrss_mem_atu_map_struct.attribute.as_uint32 = ddrss_test_atu_root_attr.as_uint32;
    PLATFORM_MEMCPY(&ddr_atu_tmp, &ddrss_mem_atu_map_struct, sizeof(atu_map_entry_t));
    if (atu_find_map(ATU_ID_MSCP, &ddr_atu_tmp) == SILIBS_SUCCESS)
    {
        offset = ddr_atu_tmp.ap_base_address - ddrss_mem_atu_map_struct.ap_base_address;
        ddrss_mem_atu_map_struct.mscp_start_address = offset + ddr_atu_tmp.mscp_start_address;
        ddrss_mem_atu_map_struct.mscp_end_address += ddrss_mem_atu_map_struct.mscp_start_address;
        DDR_ASSERT(ddr_atu_tmp.mscp_end_address >= ddrss_mem_atu_map_struct.mscp_end_address);
    }
    else
    {
        DDR_ASSERT(!atu_map(ATU_ID_MSCP, &ddrss_mem_atu_map_struct));
        ddrss_atu_map_flags |= BIT1;
    }

    DDR_ASSERT(!ddrss_set_die_base(die_num, ddrss_mem_atu_map_struct.mscp_start_address));
    info_print("DDR mem  base: %08x\n", ddrss_mem_atu_map_struct.mscp_start_address);
}

void ddrss_err_inj_atu_unmap()
{
    if (ddrss_atu_map_flags & BIT1)
    {
        DDR_ASSERT(!atu_unmap(ATU_ID_MSCP, &ddrss_mem_atu_map_struct));
    }
}

bool ddrss_ue_ce_err_inj_validation(uint32_t mc, uint16_t BIT)
{
    bool ret = true;
    if (mc >= DDRSS_MAX_SS_NUM)
    {
        ret = false;
    }
    if (BIT > 0x3FF)
    {
        ret = false;
    }
    return ret;
}

int ddrss_ue_ce_error_injection(int32_t die_num, uint32_t mc, uint64_t p_addr, uint16_t Bit)
{
    uint32_t ret = 0;
    uint32_t err_inj_mapped_addr;
    ddrss_media_data_err_inj_info_t media_data_err_inj = {0};
    ddrss_err_inj_atu_map(die_num);
    err_inj_mapped_addr = ddrss_mem_atu_map_struct.mscp_start_address + (uint32_t)(p_addr);
    media_data_err_inj.err_rs_sym = Bit;
    media_data_err_inj.err_inj_rw = 0x01;
    media_data_err_inj.err_inj_beat = 1;
    media_data_err_inj.err_inj_cnt = 1;
    DDR_ASSERT(!ddrss_inject_media_data_err(mc, &media_data_err_inj));
    ret = MMIO_READ32(err_inj_mapped_addr);
    ddrss_err_inj_atu_unmap();
    return ret;
}