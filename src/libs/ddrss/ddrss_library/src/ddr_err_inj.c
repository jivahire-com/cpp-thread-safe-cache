//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ddr_err_inj.c
 * Source file to implement ddr error injection API functions.
 */

/*------------- Includes -----------------*/
#include "ddr_atu_map.h"

#include <FpFwUtils.h>
#include <arm_intrinsic.h> // for __DSB on Windows builds (empty define)
#include <bug_check.h>
#include <ddr_err_inj.h>
#include <ddrss.h>
#include <ddrss_lib.h>
#include <fpfw_cfg_mgr.h>
#include <health_monitor.h>
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

#define UINT64_FMT(x) ((unsigned long)((x) >> 32)), (unsigned long)(x)

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
    silibs_status_t sts;
    ddrss_media_addr_t m_addr;
    uint64_t p_addr_new;
    uint32_t tgt_mc;

    // Check that cmd_merge_en is 0 or 1
    if (config_get_cmd_merge_en() != 0 && config_get_cmd_merge_en() != 1)
    {
        printf("WARNING: cmd_merge_en config knob value should be 0 or 1 to do err_inj\n");
    }

    // For UE, check that ue_max_retry is 0 - otherwise, UE will be converted to CE
    if ((Bit & BIT0) && (config_get_ue_max_retry() != 0))
    {
        printf("WARNING: ue_max_retry config knob value should be 0 to do uncorrectable err_inj\n");
    }

    tgt_mc = DDRSS_GET_LOCAL_MC(mc) + DDRSS_MAX_MC_NUM_PER_DIE * die_num;
    if (tgt_mc != mc)
    {
        // the required DIE is not local die, the request needs to be issued on remote die
        printf("Requested MC %ld resides in remote die, adjusting MC to %ld instead\n", (unsigned long)mc, (unsigned long)tgt_mc);
        mc = tgt_mc;
    }

    // Convert PA to MA
    sts = ddrss_physical_to_media_addr(p_addr, &m_addr, &tgt_mc);
    if (sts != SILIBS_SUCCESS)
    {
        printf("Invalid DDR PA address 0x%lX%08lX\n", UINT64_FMT(p_addr));
        return;
    }

    if (tgt_mc != mc)
    {
        // the required PA does not belong to the MC, so try to adjust the PA address
        sts = ddrss_find_next_physical_addr(mc, p_addr, &p_addr_new);
        if (sts != SILIBS_SUCCESS)
        {
            printf("Could not find a valid PA address around 0x%lX%08lX on MC %ld\n",
                   (unsigned long)UINT64_FMT(p_addr),
                   (unsigned long)mc);
            return;
        }

        printf("Requested PA address belongs to MC %ld, adjusting PA to 0x%lX%08lX instead\n",
               (unsigned long)tgt_mc,
               (unsigned long)UINT64_FMT(p_addr_new));
        p_addr = p_addr_new;

        // update the media address accordingly
        sts = ddrss_physical_to_media_addr(p_addr, &m_addr, &tgt_mc);
        BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
        mc = tgt_mc;
    }

    printf("Injecting UE/CE error on die %ld, MC %ld, p_addr 0x%lX%08lX, Bit 0x%X\n",
           (unsigned long)die_num,
           (unsigned long)mc,
           UINT64_FMT(p_addr),
           Bit);

    // set up address filter so that the error will only be injected at the specified address
    ddrss_media_data_err_inj_info_t media_data_err_inj = {0};
    media_data_err_inj.addr_filter.addr_mask.col = -1;
    media_data_err_inj.addr_filter.addr_mask.row = -1;
    media_data_err_inj.addr_filter.addr_mask.bank = -1;
    media_data_err_inj.addr_filter.addr_mask.bg = -1;
    media_data_err_inj.addr_filter.addr_mask.rank = -1;
    media_data_err_inj.addr_filter.addr_match.col = m_addr.col >> 4; // remove lower 4 bits in col
    media_data_err_inj.addr_filter.addr_match.row = m_addr.row;
    media_data_err_inj.addr_filter.addr_match.bank = m_addr.bank;
    media_data_err_inj.addr_filter.addr_match.bg = m_addr.bg;
    media_data_err_inj.addr_filter.addr_match.rank = m_addr.rank;

    media_data_err_inj.err_rs_sym = Bit;
    media_data_err_inj.err_inj_rw = 0x01;
    media_data_err_inj.err_inj_beat = 1;
    media_data_err_inj.err_inj_cnt = 1;
    printf("Injecting media_data_err\n");
    sts = ddrss_inject_media_data_err(mc, &media_data_err_inj);
    BUG_ASSERT_PARAM(sts == SILIBS_SUCCESS, sts, 0);
    __DSB();

    uint64_t p_addr_8K_aligned = p_addr & 0xFFFFFFFFFFFFE000;
    uint32_t p_addr_offset = (uint32_t)(p_addr & 0x1FFF);
    uint32_t scp_atu_aligned_addr = ddrss_atu_map_media_addr(p_addr_8K_aligned);
    printf("Reading data from mapped media address 0x%lX\n", (unsigned long)(scp_atu_aligned_addr + p_addr_offset));
    MMIO_READ32(scp_atu_aligned_addr + p_addr_offset);
    __DSB();

    printf("Unmapping ATU media address\n");
    ddrss_atu_unmap_media_addr(p_addr_8K_aligned);
}

acpi_einj_cmd_status_t ddr_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(einj_payload);
    FPFW_UNUSED(ctx);

    return ACPI_EINJ_SUCCESS;
}