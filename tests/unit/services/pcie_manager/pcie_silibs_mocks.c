//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Silicon library mock function implementations for pciess driver tests
 */

/*------------- Includes -----------------*/
#include <FpFwCMocka.h>       // IWYU pragma: keep
#include <cmocka.h>           // IWYU pragma: keep
#include <e32_mem_map_regs.h> // for e32_mem_map_reg
#include <idsw.h>             // for platform ID declarations
#include <idsw_kng.h>         // for KNG_PLAT_ID
#include <intu_lib.h>
#include <kng_soc_constants.h>     // for RPSS_INSTANCE
#include <pcie_knobs.h>            // for pcie_cfg_t
#include <pcie_ss_common.h>        // for pcie_ss_entity_t
#include <pcie_x16_e32_phy_regs.h> // for pcie_x16_e32_phy_reg
#include <pcie_x16_general_regs.h> // for pcie_x16_general_reg
#include <pciess_int.h>            // for INTU_DEST_PIN
#include <rpss_p1_regs.h>          // for rpss_p1_reg
#include <silibs_status.h>         // for silibs_status_t
#include <stdbool.h>               // for bool
#include <stdint.h>                // for uintptr_t, uint64_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
KNG_PLAT_ID __wrap_idsw_get_platform_sdv(void)
{
    return mock_type(KNG_PLAT_ID);
}

pcie_ss_entity_t* __wrap_pciess_get_entity(RPSS_INSTANCE rpss_idx)
{
    check_expected(rpss_idx);
    return mock_ptr_type(pcie_ss_entity_t*);
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

    info->rp_ints[0].ints[PCIESS_RP_INT_LINK_DOWN].asserted = true;
    info->rp_ints[0].ints[PCIESS_RP_INT_LINK_UP].asserted = true;
    info->rp_ints[0].ints[PCIESS_RP_INT_DPC].asserted = true;

    (void)(dest);

    return mock_type(bool);
}