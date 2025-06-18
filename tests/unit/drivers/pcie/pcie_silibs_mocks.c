//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Silicon library mock function implementations for pciess driver tests
 */

/*------------- Includes -----------------*/
#include "pcie_silibs_mocks.h"

#include <FpFwCMocka.h>       // IWYU pragma: keep
#include <FpFwUtils.h>        // for FPFW_UNUSED
#include <atu_lib.h>          // for atu_id_t, atu_map_entry_t
#include <cmocka.h>           // IWYU pragma: keep
#include <e32_mem_map_regs.h> // for e32_mem_map_reg
#include <idsw.h>             // for platform ID declarations
#include <idsw_kng.h>         // for KNG_PLAT_ID
#include <intu_lib.h>
#include <kng_soc_constants.h> // for RPSS_INSTANCE
#include <oi_pcie.h>           // for  pcie_laattr_ovrd_t
#include <pcie_knobs.h>        // for pcie_cfg_t
#include <pcie_phy.h>
#include <pcie_rp_rasdes.h>
#include <pcie_ss_common.h>        // for pcie_ss_entity_t
#include <pcie_x16_e32_phy_regs.h> // for pcie_x16_e32_phy_reg
#include <pcie_x16_general_regs.h> // for pcie_x16_general_reg
#include <pciess_int.h>            // for INTU_DEST_PIN
#include <rpss_p1_regs.h>          // for rpss_p1_reg
#include <silibs_status.h>         // for silibs_status_t
#include <stdbool.h>               // for bool
#include <stdint.h>                // for uintptr_t, uint64_t
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static pcie_x16_general_reg mock_general_reg_block;
static pcie_x16_e32_phy_reg mock_phy_block;
static e32_mem_map_reg mock_bcast_block;
static rpss_p1_reg mock_p1_block;
pciess_int_probe_t mock_int_info;

/*------------- Functions ----------------*/
idsw_die_id_t __wrap_idsw_get_die_id(void)
{
    return mock_type(idsw_die_id_t);
}

KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
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

    return mock_ptr_type(pcie_ss_entity_t*);
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

silibs_status_t __wrap_pciess_phys_program_fw(pcie_ss_entity_t* ss, pcie_phy_fw_t* fw)
{
    assert_non_null(ss);
    assert_non_null(fw);
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

silibs_status_t __wrap_pciess_rps_clear_intus(pcie_ss_entity_t* ss)
{
    assert_non_null(ss);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_rp_clear_intus(pcie_rp_entity_t* rp)
{
    assert_non_null(rp);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_rp_initiate_link_training(pcie_rp_entity_t* rp)
{
    assert_non_null(rp);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_rp_get_link_train_done(pcie_rp_entity_t* rp)
{
    assert_non_null(rp);
    return mock_type(silibs_status_t);
}

int __wrap_intu_get_interrupt_status(uintptr_t intu_base_addr, uint32_t* intr)
{
    assert_non_null(intu_base_addr);
    assert_non_null(intr);
    *intr = mock();
    return 0;
}

bool __wrap_pciess_probe(pcie_ss_entity_t* ss, pciess_int_probe_t* info, INTU_DEST_PIN dest)
{
    assert_non_null(info);
    assert_non_null(ss);

    memcpy(info, &mock_int_info, sizeof(pciess_int_probe_t));
    (void)(dest);

    return mock_type(bool);
}

silibs_status_t __wrap_oi_pcie_rp_dbi_hide_dpc_cap(pcie_rp_entity_t* rp)
{
    assert_non_null(rp);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pciess_rp_ready(pcie_rp_entity_t* rp)
{
    assert_non_null(rp);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_oi_pcie_rp_dbi_set_read_cacheability(pcie_rp_entity_t* rp,
                                                            uint8_t domain_mode,
                                                            uint8_t domain_value,
                                                            uint8_t cache_mode,
                                                            uint8_t cache_value)
{
    assert_non_null(rp);
    FPFW_UNUSED(domain_mode);
    FPFW_UNUSED(domain_value);
    FPFW_UNUSED(cache_mode);
    FPFW_UNUSED(cache_value);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_oi_pcie_rp_dbi_set_write_cacheability(pcie_rp_entity_t* rp,
                                                             uint8_t domain_mode,
                                                             uint8_t domain_value,
                                                             uint8_t cache_mode,
                                                             uint8_t cache_value)
{
    assert_non_null(rp);
    FPFW_UNUSED(domain_mode);
    FPFW_UNUSED(domain_value);
    FPFW_UNUSED(cache_mode);
    FPFW_UNUSED(cache_value);
    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_oi_pcie_ss_set_laattr_rp_overrides(pcie_ss_entity_t* ss, unsigned rp_index, pcie_laattr_ovrd_t* overrides, bool wr_rd)
{
    assert_non_null(ss);
    assert_non_null(overrides);
    FPFW_UNUSED(wr_rd);
    FPFW_UNUSED(rp_index);
    return mock_type(silibs_status_t);
}

int __wrap_pcie_rp_vsecras_clear_rasdp_error_mode(pcie_rp_entity_t* rp)
{
    assert_non_null(rp);
    return mock_type(silibs_status_t);
}

bool __wrap_ras_pcie_vsecras_agent_probe(ras_agent_entity_t* agent, ras_error_record_t* record)
{
    assert_non_null(agent);
    assert_non_null(record);
    return mock_type(bool);
}

bool __wrap_ras_pcie_dtim_agent_probe(ras_agent_entity_t* agent, ras_error_record_t* record)
{
    assert_non_null(agent);
    assert_non_null(record);
    return mock_type(bool);
}

bool __wrap_ras_pcie_ltim_agent_probe(ras_agent_entity_t* agent, ras_error_record_t* record)
{
    assert_non_null(agent);
    assert_non_null(record);
    return mock_type(bool);
}

silibs_status_t __wrap_pcie_ss_rp_aer_spoof(pcie_ss_entity_t* ss, unsigned rp_index, uint32_t app_errors)
{
    assert_non_null(ss);
    FPFW_UNUSED(app_errors);
    assert_in_range(rp_index, 0, 3);

    return mock_type(silibs_status_t);
}

int __wrap_pcie_rp_rasdes_inject_crc_error(pcie_rp_entity_t* rp, PCIE_RASDES_INJ_CRC_TYPE type, uint8_t count)
{
    assert_non_null(rp);
    FPFW_UNUSED(type);
    assert_in_range(count, 1, 255);

    return mock_type(silibs_status_t);
}

int __wrap_pcie_rp_rasdes_inject_seqnum_error(pcie_rp_entity_t* rp, PCIE_RASDES_INJ_SEQNUM_TYPE type, uint8_t count)
{
    assert_non_null(rp);
    FPFW_UNUSED(type);
    assert_in_range(count, 1, 255);

    return mock_type(silibs_status_t);
}
int __wrap_pcie_rp_rasdes_inject_dllp_error(pcie_rp_entity_t* rp, PCIE_RASDES_INJ_DLLP_TYPE type, uint8_t count)
{
    assert_non_null(rp);
    FPFW_UNUSED(type);
    assert_in_range(count, 1, 255);

    return mock_type(silibs_status_t);
}
int __wrap_pcie_rp_rasdes_inject_symbol_error(pcie_rp_entity_t* rp, PCIE_RASDES_INJ_SYMBOL_TYPE type, uint8_t count)
{
    assert_non_null(rp);
    FPFW_UNUSED(type);
    assert_in_range(count, 1, 255);

    return mock_type(silibs_status_t);
}

int __wrap_pcie_rp_rasdes_inject_fc_error(pcie_rp_entity_t* rp, PCIE_RASDES_INJ_FC_TYPE type, uint8_t count)
{
    assert_non_null(rp);
    FPFW_UNUSED(type);
    assert_in_range(count, 1, 255);

    return mock_type(silibs_status_t);
}

int __wrap_pcie_rp_rasdes_inject_retry_tlp_error(pcie_rp_entity_t* rp, PCIE_RASDES_INJ_RETRY_TLP_TYPE type, uint8_t count)
{
    assert_non_null(rp);
    FPFW_UNUSED(type);
    assert_in_range(count, 1, 255);

    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pcie_phy_setup_error_injection(uintptr_t base, PCIE_PHY_INST inst, uint32_t inj_mask, uint16_t addr_low, uint16_t addr_high)
{
    assert_non_null(base);
    assert_in_range(inst, PCIE_PHY_0, PCIE_PHY_BCAST);
    FPFW_UNUSED(inj_mask);
    FPFW_UNUSED(addr_low);
    FPFW_UNUSED(addr_high);

    return mock_type(silibs_status_t);
}

silibs_status_t __wrap_pcie_phy_clear_injection(uintptr_t base, PCIE_PHY_INST inst)
{
    assert_non_null(base);
    assert_in_range(inst, PCIE_PHY_0, PCIE_PHY_BCAST);

    return mock_type(silibs_status_t);
}
