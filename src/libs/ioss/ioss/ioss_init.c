//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file ioss_init.c
 *  This modules initializes IOSS
 */

/*------------- Includes -----------------*/
#include "ioss_init.h"

#include <FpFwAssert.h>             // for FPFW_RUNTIME_ASSERT
#include <atu_lib.h>                // for atu_map_entry_t, atu_map, atu_unmap
#include <ioss_top_regs.h>          // for IOSS_TOP_IOSS_PCR_ADDRESS, IOSS_...
#include <kng_soc_constants.h>      // for ATU_PAGE_SIZE, NUM_IOSS_INSTANCES
#include <pcr_clock_config.h>       // for PCR_CLOCK_SELECT_B
#include <pcr_ioss.h>               // for program_ioss_pcr_clock_mux, prog...
#include <silibs_ap_top_regs.h>     // for AP_TOP_D0_VAB_CDED_IOSS_ADDRESS
#include <silibs_common.h>          // for ALIGN_UP
#include <silibs_status.h>          // for SILIBS_SUCCESS
#include <stdint.h>                 // for uint64_t, uint32_t, uint8_t
#include <tower_ioss.h>             // for configure_ioss_system_addr_map
#include <vab_cded_ioss_top_regs.h> // for VAB_CDED_IOSS_TOP_IOSS_ADDRESS

/*-- Symbolic Constant Macros (defines) --*/
#define SCP_CORE_CLOCK_FREQUENCY 1000000000 // 1GHz

/*------------- Typedefs -----------------*/

/*-- Declarations (Statics and globals) --*/
static uint64_t ioss_addrs[NUM_IOSS_INSTANCES] = {
    AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
    AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
};

static atu_map_entry_t atu_ioss_map[NUM_IOSS_INSTANCES] = {
    {
        // D0-IOSS0
        .ap_base_address = AP_TOP_D0_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(VAB_CDED_IOSS_TOP_IOSS_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {{.axprot0 = 0x3, .axprot1 = 0x2, .axnse = 0x3}},
    },
    {
        // D1-IOSS0
        .ap_base_address = AP_TOP_D1_VAB_CDED_IOSS_ADDRESS + VAB_CDED_IOSS_TOP_IOSS_ADDRESS,
        .mscp_start_address = 0,
        .mscp_end_address = ALIGN_UP(VAB_CDED_IOSS_TOP_IOSS_SIZE, ATU_PAGE_SIZE) - 1,
        .attribute = {{.axprot0 = 0x3, .axprot1 = 0x2, .axnse = 0x3}},
    },
};

/*------------- Functions ----------------*/
void ioss_init(uint8_t die_num)
{
    FPFW_RUNTIME_ASSERT(die_num < NUM_DIE);

    int sts = atu_map(ATU_ID_MSCP, &atu_ioss_map[die_num]);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    uint32_t resolved_ioss_base_addr = atu_ioss_map[die_num].mscp_start_address;

    // IOSS tower
    tower_configure_ioss_apu(ioss_addrs[die_num], (resolved_ioss_base_addr + IOSS_TOP_NOCNI_NITOWER_IOSS_ADDRESS));
    configure_ioss_system_addr_map(ioss_addrs[die_num], (resolved_ioss_base_addr + IOSS_TOP_NOCNI_NITOWER_IOSS_ADDRESS));

    // IOSS PCR init
    // Enable USBSS
    program_ioss_pcr_usb_reset(resolved_ioss_base_addr + IOSS_TOP_IOSS_PCR_ADDRESS);

    // TODO: Silibs WI 450450 Confirm SCP core clk freq, PLL lock & mux config timeout based on new PLL spec
    // 5us timeout based on scp core clock frequency
    const uint32_t mux_config_timeout = ((unsigned int)(5 * (SCP_CORE_CLOCK_FREQUENCY / 1000000)));

    sts = program_ioss_pcr_clock_mux(resolved_ioss_base_addr + IOSS_TOP_IOSS_PCR_ADDRESS, IOSSFAM_GFMUX, PCR_CLOCK_SELECT_B, mux_config_timeout);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);

    sts = atu_unmap(ATU_ID_MSCP, &atu_ioss_map[die_num]);
    FPFW_RUNTIME_ASSERT(sts == SILIBS_SUCCESS);
}