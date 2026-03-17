//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_err_domain_common.cpp
 * Tests the common functionalities of the error domain
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_int_equal, assert_non_null, expe...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <dmac.h>
#include <idsw_kng.h> // for KNG_DIE_ID, idsw_get_die_id
#include <interrupts.h>
#include <kng_atu_mappings.h>
#include <mscp_error_domain.h>
#include <mscp_exp_rmss_memory_map.h>
#include <scp_exp_csr_regs.h>
#include <scp_exp_top_regs.h>
#define __NO_LARGE_ADDRMAP_TYPEDEFS__
#include <scp_top_regs.h>
#include <shared_sram_ecc_ras_registers_regs.h>
#include <stdint.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
//
// Mocks
//
idsw_cpu_type_t __wrap_idsw_get_cpu_type(void)
{
    return mock_type(idsw_cpu_type_t);
}

void __wrap_dmac_init(uintptr_t dmac_base_addr, uint32_t timeout)
{
    check_expected(dmac_base_addr);
    FPFW_UNUSED(timeout);
    function_called();
}

int __wrap_dmac_start_single_block_transfer(uintptr_t dmac_base_addr, dmac_cfg_t* cfg)
{
    check_expected(dmac_base_addr);
    check_expected(cfg->dmac_dest_tr_width);
    check_expected(cfg->dmac_src_tr_width);
    check_expected(cfg->max_block_ts_bytes);
    check_expected(cfg->dmac_transfer_cfg->channel_id);
    check_expected(cfg->dmac_transfer_cfg->dmac_dest_addr);
    check_expected(cfg->dmac_transfer_cfg->dmac_src_addr);
    check_expected(cfg->dmac_transfer_cfg->transfer_size);
    function_called();
    return mock_type(int);
}

void __wrap_dmac_wait_for_transfer_complete(uintptr_t dmac_base_addr, DMAC_CHANNEL channel_id, uint32_t timeout)
{
    check_expected(dmac_base_addr);
    check_expected(channel_id);
    FPFW_UNUSED(timeout);
    function_called();
}
}
//
// Tests
//

TEST_FUNCTION(test_clear_poison_memory, nullptr, nullptr)
{
    uintptr_t expected_poison_mem_base_addr = 0x100;
    uintptr_t expected_scp_dmac_base_addr = SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_DMAC_ADDRESS;
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);
    expect_value(__wrap_dmac_init, dmac_base_addr, expected_scp_dmac_base_addr);
    expect_function_call(__wrap_dmac_init);

    expect_value(__wrap_dmac_start_single_block_transfer, dmac_base_addr, expected_scp_dmac_base_addr);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_dest_tr_width, DMAC_TRANSFER_WIDTH_64_BITS);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_src_tr_width, DMAC_TRANSFER_WIDTH_64_BITS);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->max_block_ts_bytes, DMAC_MAX_BLOCK_TS_BYTES);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_transfer_cfg->channel_id, DMAC_CHANNEL0);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_transfer_cfg->dmac_dest_addr, expected_poison_mem_base_addr);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_transfer_cfg->dmac_src_addr, SCP_EXP_RESERVED_POISON_CLEAR_BASE);
    expect_value(__wrap_dmac_start_single_block_transfer, cfg->dmac_transfer_cfg->transfer_size, sizeof(uint64_t));
    expect_function_call(__wrap_dmac_start_single_block_transfer);
    will_return(__wrap_dmac_start_single_block_transfer, SILIBS_SUCCESS);

    expect_value(__wrap_dmac_wait_for_transfer_complete, dmac_base_addr, expected_scp_dmac_base_addr);
    expect_value(__wrap_dmac_wait_for_transfer_complete, channel_id, DMAC_CHANNEL0);
    expect_function_call(__wrap_dmac_wait_for_transfer_complete);

    clear_poison_memory(expected_poison_mem_base_addr);
}