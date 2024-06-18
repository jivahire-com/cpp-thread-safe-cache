//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Silicon library mock function implementations for pciess driver tests
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h>            // IWYU pragma: keep
#include <atu_lib.h>               // for atu_id_t, atu_map_entry_t
#include <cmocka.h>                // IWYU pragma: keep
#include <e32_mem_map_regs.h>      // for e32_mem_map_reg
#include <idsw.h>                  // for PLAT_ID
#include <kng_soc_constants.h>     // for RPSS_INSTANCE
#include <pcie_knobs.h>            // for pcie_cfg_t
#include <pcie_ss_common.h>        // for pcie_ss_entity_t
#include <pcie_x16_e32_phy_regs.h> // for pcie_x16_e32_phy_reg
#include <pcie_x16_general_regs.h> // for pcie_x16_general_reg
#include <rpss_p1_regs.h>          // for rpss_p1_reg
#include <silibs_status.h>         // for silibs_status_t
#include <stdbool.h>               // for bool
#include <stdint.h>                // for uintptr_t, uint64_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pcie_ss_entity_t mock_pcie_ent;
static pcie_x16_general_reg mock_general_reg_block;
static pcie_x16_e32_phy_reg mock_phy_block;
static e32_mem_map_reg mock_bcast_block;
static rpss_p1_reg mock_p1_block;

/*------------- Functions ----------------*/
PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(PLAT_ID);
}

int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    check_expected(atu_id);

    /* Keep mscp base zero to allow checking base address in UTs */
    atu_map_entry->mscp_start_address = 0x0;

    return mock_type(int);
}

pcie_ss_entity_t* __wrap_pciess_get_entity(RPSS_INSTANCE rpss_idx)
{
    check_expected(rpss_idx);

    mock_pcie_ent.id = rpss_idx;

    return &(mock_pcie_ent);
}

silibs_status_t __wrap_pciess_config_entity(pcie_ss_entity_t* ss,
                                            uintptr_t rpss_base_addr,
                                            uint64_t dbi_base_addr,
                                            pcie_cfg_t* cfg,
                                            bool program_phy_regs,
                                            bool enable_apu)
{
    assert_non_null(ss);
    assert_non_null(dbi_base_addr);
    assert_non_null(cfg);
    check_expected(program_phy_regs);
    check_expected(enable_apu);

    ss->knobs = cfg;
    ss->bases.general_base_addr = (uintptr_t)(&mock_general_reg_block);
    ss->bases.phy_base_addr = (uintptr_t)(&mock_phy_block);
    ss->bases.phy_bcast_base_addr = (uintptr_t)(&mock_bcast_block);
    ss->bases.p1_base_addr = (uintptr_t)(&mock_p1_block);
    ss->bases._base = rpss_base_addr;

    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_config_ss_for_bifur(pcie_ss_entity_t* ss)
{
    assert_non_null(ss);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_deassert_por_reset(pcie_ss_entity_t* ss)
{
    assert_non_null(ss);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_phys_sram_init_done(pcie_ss_entity_t* ss)
{
    assert_non_null(ss);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_rps_pre_rp_ready_init(pcie_ss_entity_t* ss)
{
    assert_non_null(ss);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_rps_ready(pcie_ss_entity_t* ss)
{
    assert_non_null(ss);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_rps_post_rp_ready_init(pcie_ss_entity_t* ss)
{
    assert_non_null(ss);
    return mock_type(silibs_status_t);
}
