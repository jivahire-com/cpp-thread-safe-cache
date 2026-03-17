//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file error_domain_util.c
 * This file contains utility functions for error domain management.
 */

/*------------- Includes -----------------*/
#include <DbgPrint.h>
#include <FpFwUtils.h>
#include <dmac.h>
#include <error_domain_i.h>
#define __NO_CSR_TYPEDEFS__
#define __NO_ADDRMAP_TYPEDEFS__
#include <mcp_exp_top_regs.h>
#include <mscp_exp_rmss_memory_map.h>
#include <scp_exp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#undef __NO_ADDRMAP_TYPEDEFS__
#include <silibs_common.h>

/*-- Symbolic Constant Macros (defines) --*/

/*-------- Function Prototypes -----------*/

/*-------------- Typedefs ----------------*/

/*-- Declarations (Statics and globals) --*/

/*-------------- Functions ---------------*/
int clear_poison_memory(uintptr_t poison_addr)
{
    memset((void*)SCP_EXP_RESERVED_POISON_CLEAR_BASE, 0, SCP_EXP_RESERVED_POISON_CLEAR_SIZE);
    __DMB();

    uint64_t dmac_base_addr = 0;

    if (idsw_get_cpu_type() == CPU_SCP)
    {
        dmac_base_addr = DMAC_SCP_BASE_ADDR;
    }
    else if (idsw_get_cpu_type() == CPU_MCP)
    {
        dmac_base_addr = DMAC_MCP_BASE_ADDR;
    }

    dmac_init(dmac_base_addr, DMAC_RESET_TIMEOUT);

    // Configure DMA transfer
    dmac_transfer_cfg_t dmac_tr_cfg = {0};
    dmac_tr_cfg.channel_id = DMAC_CHANNEL0;
    dmac_tr_cfg.dmac_dest_addr = ALIGN_DOWN(poison_addr, 8);
    dmac_tr_cfg.dmac_src_addr = SCP_EXP_RESERVED_POISON_CLEAR_BASE;
    dmac_tr_cfg.transfer_size = SCP_EXP_RESERVED_POISON_CLEAR_SIZE;

    dmac_cfg_t dmac_cfg = {0};
    dmac_cfg.dmac_transfer_cfg = &dmac_tr_cfg;
    dmac_cfg.max_block_ts_bytes = DMAC_MAX_BLOCK_TS_BYTES;
    dmac_cfg.dmac_src_tr_width = DMAC_TRANSFER_WIDTH_64_BITS;
    dmac_cfg.dmac_dest_tr_width = DMAC_TRANSFER_WIDTH_64_BITS;

    silibs_status_t status = dmac_start_single_block_transfer(dmac_base_addr, &dmac_cfg);

    if (SILIBS_SUCCESS != status)
    {
        FPFW_DBGPRINT_ERROR("Fail to start DMAC transfer (%d)\n", status);
    }
    else
    {
        dmac_wait_for_transfer_complete(dmac_base_addr, dmac_tr_cfg.channel_id, MSCP_DMAC_TRANSFER_TIMEOUT);
    }

    return status;
}
