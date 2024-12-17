//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_err_inj.c
 * Source file to implement ddr error injection API functions.
 */

/*------------- Includes -----------------*/
#include "ddr_atu_map.h"

#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <ddr_err_inj.h>
#include <ddrss.h>
#include <ddrss_lib.h>
#include <kng_soc_constants.h>
#include <nvic.h> // Has nested include of cmsis_gcc_m.h for __DSB() intrinsic
#include <silibs_ap_top_regs.h>
#include <silibs_platform.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DDR_ASSERT ASSERT_FAIL

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*---------------- Function ---------------*/
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

void ddrss_ue_ce_error_injection(int32_t die_num, uint32_t mc, uint64_t p_addr, uint16_t Bit)
{
    uint32_t err_inj_mapped_addr = ddrss_atu_map(die_num);

    // TODO: Maurice says the "err_inj_mapped_addr" should be DDRSS media address instead of DDRSS CFG space
    // address ADO: 2199710
    err_inj_mapped_addr += (uint32_t)(p_addr);

    ddrss_media_data_err_inj_info_t media_data_err_inj = {0};
    media_data_err_inj.err_rs_sym = Bit;
    media_data_err_inj.err_inj_rw = 0x01;
    media_data_err_inj.err_inj_beat = 1;
    media_data_err_inj.err_inj_cnt = 1;
    DDR_ASSERT(!ddrss_inject_media_data_err(mc, &media_data_err_inj));
    __DSB();

    MMIO_READ32(err_inj_mapped_addr);

    ddrss_atu_unmap(die_num);
}
