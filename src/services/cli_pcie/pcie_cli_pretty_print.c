//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file
 * Print helpers pretty-printing PCIe cli responses
 */

/*------------- Includes -----------------*/
#include <FpFwCli.h>
#include <kng_soc_constants.h>
#include <pcie_cli_i.h>
#include <pcie_common.h>
#include <pcie_rp_common.h>
#include <pcie_ss_common.h>
#include <rc4sx16_pf0_type1_hdr_regs.h>
#include <silibs_kng_soc.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
static char* pcie_width_str[6] = {
    "PCIE_WIDTH_X1",
    "PCIE_WIDTH_X2",
    "PCIE_WIDTH_X4",
    "PCIE_WIDTH_X8",
    "PCIE_WIDTH_X16",
    "UNKNOWN_WIDTH",
};

static char* pcie_ltssm_state_str[PCIE_LTSSM_STATE_RECOVERY_EQUALIZATION_3 + 2] = {
    "PCIE_LTSSM_STATE_DETECT_QUIET",
    "PCIE_LTSSM_STATE_DETECT_ACTIVE",
    "PCIE_LTSSM_STATE_POLLING_ACTIVE",
    "PCIE_LTSSM_STATE_POLLING_COMPLIANCE",
    "PCIE_LTSSM_STATE_POLLING_CONFIGURATION",
    "PCIE_LTSSM_STATE_PRE_DETECT_QUIET",
    "PCIE_UNKNOWN_LTSSM_STATE_0x06",
    "PCIE_LTSSM_STATE_CONFIGURATION_LINKWIDTH_START",
    "PCIE_LTSSM_STATE_CONFIGURATION_LINKWDITH_ACCEPT",
    "PCIE_LTSSM_STATE_CONFIGURATION_LANENUM_WAIT",
    "PCIE_LTSSM_STATE_CONFIGURATION_LANENUM_ACCEPT",
    "PCIE_LTSSM_STATE_CONFIGURATION_COMPLETE",
    "PCIE_LTSSM_STATE_CONFIGURATION_IDLE",
    "PCIE_LTSSM_STATE_RECOVERY_RCVRLOCK",
    "PCIE_LTSSM_STATE_RECOVERY_SPEED",
    "PCIE_LTSSM_STATE_RECOVERY_RCVRCFG",
    "PCIE_LTSSM_STATE_RECOVERY_IDLE",
    "PCIE_LTSSM_STATE_L0",
    "PCIE_LTSSM_STATE_L0S",
    "PCIE_LTSSM_STATE_L1_ENTRY",
    "PCIE_LTSSM_STATE_L1_IDLE",
    "PCIE_LTSSM_STATE_L2_IDLE",
    "PCIE_LTSSM_STATE_L2_TRANSMIT_WAKE",
    "PCIE_LTSSM_STATE_DISABLED_0x17",
    "PCIE_LTSSM_STATE_DISABLED_0x18",
    "PCIE_LTSSM_STATE_DISABLED_0x19",
    "PCIE_LTSSM_STATE_LOOPBACK_ENTRY",
    "PCIE_LTSSM_STATE_LOOPBACK_ACTIVE",
    "PCIE_LTSSM_STATE_LOOPBACK_EXIT",
    "PCIE_LTSSM_STATE_HOT_RESET_0x1E",
    "PCIE_LTSSM_STATE_HOT_RESET_0x1F",
    "PCIE_LTSSM_STATE_RECOVERY_EQUALIZATION_0",
    "PCIE_LTSSM_STATE_RECOVERY_EQUALIZATION_1",
    "PCIE_LTSSM_STATE_RECOVERY_EQUALIZATION_2",
    "PCIE_LTSSM_STATE_RECOVERY_EQUALIZATION_3",
    "PCIE_UNKNOWN_LTSSM_STATE"};

/*------------- Functions ----------------*/
static char* get_link_width_str(PCIE_PORT_WIDTH width)
{
    char* ret;

    switch (width)
    {
    case PCIE_WIDTH_X1:
        ret = pcie_width_str[0];
        break;
    case PCIE_WIDTH_X2:
        ret = pcie_width_str[1];
        break;
    case PCIE_WIDTH_X4:
        ret = pcie_width_str[2];
        break;
    case PCIE_WIDTH_X8:
        ret = pcie_width_str[3];
        break;
    case PCIE_WIDTH_X16:
        ret = pcie_width_str[4];
        break;
    default:
        ret = pcie_width_str[5];
        break;
    }
    return ret;
}

void print_rpss_entity(pcie_ss_entity_t* ss)
{
    FpFwCliPrint("======= PCIe RPSS [%d] entity dump start ========\n", (uint8_t)ss->id);
    FpFwCliPrint("type:        %s\n", (ss->ss_type == PCIE_SS_X16) ? "PCIE_SS_X16" : "PCIE_SS_X8");
    FpFwCliPrint("valid:       %s\n", (ss->valid == true) ? "true" : "false");
    FpFwCliPrint("live:        %s\n", (ss->live == true) ? "true" : "false");
    FpFwCliPrint("fuses_read:  %s\n", (ss->fuses_read == true) ? "true" : "false");
    FpFwCliPrint("degraded:    %s\n", (ss->degraded == true) ? "true" : "false");
    FpFwCliPrint("============== Bases ============================\n");
    FpFwCliPrint("_base:                0x%lx\n", (uint32_t)(ss->bases._base));
    FpFwCliPrint("general_base_addr:    0x%lx\n", (uint32_t)(ss->bases.general_base_addr));
    FpFwCliPrint("phy_base_addr:        0x%lx\n", (uint32_t)(ss->bases.phy_base_addr));
    FpFwCliPrint("phy_bcast_base_addr:  0x%lx\n", (uint32_t)(ss->bases.phy_bcast_base_addr));
    FpFwCliPrint("p1_base_addr:         0x%lx\n", (uint32_t)(ss->bases.p1_base_addr));
    FpFwCliPrint("============== RPs ==============================\n");
    FpFwCliPrint("rp[0].valid:   %s\n", (ss->rps[0].valid == true) ? "true" : "false");
    FpFwCliPrint("rp[0].live:    %s\n", (ss->rps[0].live == true) ? "true" : "false");
    FpFwCliPrint("rp[1].valid:   %s\n", (ss->rps[1].valid == true) ? "true" : "false");
    FpFwCliPrint("rp[1].live:    %s\n", (ss->rps[1].live == true) ? "true" : "false");
    FpFwCliPrint("rp[2].valid:   %s\n", (ss->rps[2].valid == true) ? "true" : "false");
    FpFwCliPrint("rp[2].live:    %s\n", (ss->rps[2].live == true) ? "true" : "false");
    FpFwCliPrint("rp[3].valid:   %s\n", (ss->rps[3].valid == true) ? "true" : "false");
    FpFwCliPrint("rp[3].live:    %s\n", (ss->rps[3].live == true) ? "true" : "false");
    FpFwCliPrint("================= End ===========================\n");
}

void print_rp_entity(pcie_rp_entity_t* rp)
{
    FpFwCliPrint("======= PCIe RPSS [%d] RP [%d] entity dump start ========\n",
                 (uint8_t)rp->ss_id,
                 (uint8_t)rp->id);
    FpFwCliPrint("type:     %s\n",
                 (rp->rp_type == PCIE_RP_X16) ? "PCIE_SS_X16" : ((rp->rp_type == PCIE_RP_X8) ? "PCIE_RP_X8" : "PCIE_RP_X4"));
    FpFwCliPrint("live:     %s\n", (rp->live == true) ? "true" : "false");
    FpFwCliPrint("valid:    %s\n", (rp->valid == true) ? "true" : "false");
    FpFwCliPrint("enabled:  %s\n", (rp->enabled == true) ? "true" : "false");
    FpFwCliPrint("============== Bases ====================================\n");
    FpFwCliPrint("_base:           0x%lx\n", (uint32_t)(rp->bases._base));
    FpFwCliPrint("phy_base_addr:   0x%lx\n", (uint32_t)(rp->bases.phy_base_addr));
    FpFwCliPrint("sii_base_addr:   0x%lx\n", (uint32_t)(rp->bases.sii_base_addr));
    FpFwCliPrint("dbi_base_addr:   0x%lx\n", (uint32_t)(rp->bases.dbi_base_addr));
    FpFwCliPrint("cfg_dbi_base:    0x%lx\n", (uint32_t)(rp->bases.cfg_dbi_base));
    FpFwCliPrint("============== Bus Info =================================\n");
    FpFwCliPrint("primary_bus:     0x%x\n", rp->bus_cfg.primary_bus);
    FpFwCliPrint("secondary_bus:   0x%x\n", rp->bus_cfg.secondary_bus);
    FpFwCliPrint("subordinate_bus: 0x%x\n", rp->bus_cfg.subordinate_bus);
    FpFwCliPrint("============== Range Info ===============================\n");
    if (rp->ranges != NULL)
    {
        FpFwCliPrint("valid:        %s\n", (rp->ranges->valid == true) ? "true" : "false");
        FpFwCliPrint("ecam_start:   0x%llx\n", rp->ranges->ecam_start);
        FpFwCliPrint("ecam_end:     0x%llx\n", rp->ranges->ecam_end);
        FpFwCliPrint("mmiol_start:  0x%llx\n", rp->ranges->mmiol_start);
        FpFwCliPrint("mmiol_end:    0x%llx\n", rp->ranges->mmiol_end);
        FpFwCliPrint("mmioh_start:  0x%llx\n", rp->ranges->mmioh_start);
        FpFwCliPrint("mmioh_end:    0x%llx\n", rp->ranges->mmioh_end);
    }
    else
    {
        FpFwCliPrint("Ranges uninitialized!\n");
    }
    FpFwCliPrint("============== Link State ===============================\n");
    FpFwCliPrint("Target Speed           : Gen%d\n", rp->target_state.linkrate);
    FpFwCliPrint("Current Speed          : Gen%d\n", rp->current_state.linkrate);
    FpFwCliPrint("Target Width           : %s\n", get_link_width_str(rp->target_state.lanecount));
    FpFwCliPrint("Current Width          : %s\n", get_link_width_str(rp->current_state.lanecount));
    FpFwCliPrint("Target raw lane count  : %d\n", rp->target_state.raw_lanecount);
    FpFwCliPrint("Current raw lane count : %d\n", rp->current_state.raw_lanecount);
    FpFwCliPrint("================= End ===================================\n");
}

void print_link_state(RPSS_INSTANCE rpss_idx, uint8_t rp_idx, PCIE_LTSSM_STATE ltssm_state, pcie_link_state_t* link_state)
{
    if (link_state == NULL)
    {
        FpFwCliPrint("PCIe RPSS [%d] RP [%d] link state is NULL!\n", (uint8_t)rpss_idx, (uint8_t)rp_idx);
        return;
    }

    bool unknown_ltssm_state = false;
    if (ltssm_state > PCIE_LTSSM_STATE_RECOVERY_EQUALIZATION_3)
    {
        unknown_ltssm_state = true;
    }

    FpFwCliPrint("======= PCIe RPSS [%d] RP [%d] link state =========\n", (uint8_t)rpss_idx, (uint8_t)rp_idx);
    FpFwCliPrint("LTSSM state              : %s [%d]\n",
                 ((unknown_ltssm_state == false) ? pcie_ltssm_state_str[ltssm_state]
                                                 : pcie_ltssm_state_str[PCIE_LTSSM_STATE_RECOVERY_EQUALIZATION_3]),
                 ltssm_state);
    FpFwCliPrint("Current Speed            : Gen%d\n", link_state->linkrate);
    FpFwCliPrint("Current Width            : %s\n", get_link_width_str(link_state->lanecount));
    FpFwCliPrint("Current Raw Lane Count   : %d\n", link_state->raw_lanecount);
    FpFwCliPrint("================== End ============================\n");
}

void print_type1_rp_header(RPSS_INSTANCE rpss_idx, uint8_t rp_idx, rc4sx16_pf0_type1_hdr_reg* rp_t1_hdr)
{
    if (rp_t1_hdr == NULL)
    {
        FpFwCliPrint("PCIe RPSS [%d] RP [%d] rp_hdr is NULL\n", (uint8_t)rpss_idx, (uint8_t)rp_idx);
        return;
    }

    /* clang-format off */
    FpFwCliPrint("======= PCIe RPSS [%d] RP [%d] type 1 config header =========\n", (uint8_t)rpss_idx, (uint8_t)rp_idx);
    FpFwCliPrint("Vendor ID                                 : 0x%lx\n", rp_t1_hdr->type1_dev_id_vend_id_reg.vendor_id);
    FpFwCliPrint("Device ID                                 : 0x%lx\n", rp_t1_hdr->type1_dev_id_vend_id_reg.device_id);
    FpFwCliPrint("Status     | Command                      : 0x%lx\n", rp_t1_hdr->type1_status_command_reg.as_uint32);
    FpFwCliPrint("Class Code | Rev ID                       : 0x%lx\n", rp_t1_hdr->type1_class_code_rev_id_reg.as_uint32);
    FpFwCliPrint("Cache Line Size                           : 0x%lx\n", rp_t1_hdr->type1_bist_hdr_type_lat_cache_line_size_reg.cache_line_size);
    FpFwCliPrint("Primary Latency Timer                     : 0x%lx\n", rp_t1_hdr->type1_bist_hdr_type_lat_cache_line_size_reg.latency_master_timer);
    FpFwCliPrint("Primary Latency Timer                     : 0x%lx\n", rp_t1_hdr->type1_bist_hdr_type_lat_cache_line_size_reg.header_type);
    FpFwCliPrint("BIST                                      : 0x%lx\n", rp_t1_hdr->type1_bist_hdr_type_lat_cache_line_size_reg.bist);
    FpFwCliPrint("BAR0                                      : 0x%lx\n", rp_t1_hdr->bar0_reg.as_uint32);
    FpFwCliPrint("BAR1                                      : 0x%lx\n", rp_t1_hdr->bar1_reg.as_uint32);
    FpFwCliPrint("Primary Bus                               : 0x%lx\n", rp_t1_hdr->sec_lat_timer_sub_bus_sec_bus_pri_bus_reg.prim_bus);
    FpFwCliPrint("Secondary Bus                             : 0x%lx\n", rp_t1_hdr->sec_lat_timer_sub_bus_sec_bus_pri_bus_reg.sec_bus);
    FpFwCliPrint("Subordinate Bus                           : 0x%lx\n", rp_t1_hdr->sec_lat_timer_sub_bus_sec_bus_pri_bus_reg.sub_bus);
    FpFwCliPrint("Secondary Latency Timer                   : 0x%lx\n", rp_t1_hdr->sec_lat_timer_sub_bus_sec_bus_pri_bus_reg.sec_lat_timer);
    FpFwCliPrint("Secondary Status | I/O Limit | I/O Base   : 0x%lx\n", rp_t1_hdr->sec_stat_io_limit_io_base_reg.as_uint32);
    FpFwCliPrint("Memory Base                               : 0x%lx\n", rp_t1_hdr->mem_limit_mem_base_reg.mem_base);
    FpFwCliPrint("Memory Limit                              : 0x%lx\n", rp_t1_hdr->mem_limit_mem_base_reg.mem_limit);
    FpFwCliPrint("Prefetchable Memory Lower Base            : 0x%lx\n", rp_t1_hdr->pref_mem_limit_pref_mem_base_reg.pref_mem_base);
    FpFwCliPrint("Prefetchable Memory Lower Limit           : 0x%lx\n", rp_t1_hdr->pref_mem_limit_pref_mem_base_reg.pref_mem_limit);
    FpFwCliPrint("Prefetchable Memory Upper Base            : 0x%lx\n", rp_t1_hdr->pref_base_upper_reg.pref_mem_base_upper);
    FpFwCliPrint("Prefetchable Memory Upper Limit           : 0x%lx\n", rp_t1_hdr->pref_limit_upper_reg.pref_mem_limit_upper);
    FpFwCliPrint("I/O Upper Limit                           : 0x%lx\n", rp_t1_hdr->io_limit_upper_io_base_upper_reg.io_base_upper);
    FpFwCliPrint("I/O Upper Base                            : 0x%lx\n", rp_t1_hdr->io_limit_upper_io_base_upper_reg.io_limit_upper);
    FpFwCliPrint("Capabilities Pointer                      : 0x%lx\n", rp_t1_hdr->type1_cap_ptr_reg.cap_pointer);
    FpFwCliPrint("Expansion ROM Base                        : 0x%lx\n", rp_t1_hdr->type1_exp_rom_base_reg.exp_rom_base_address);
    FpFwCliPrint("Interrupt Line                            : 0x%lx\n", rp_t1_hdr->bridge_ctrl_int_pin_int_line_reg.int_line);
    FpFwCliPrint("Interrupt Pin                             : 0x%lx\n", rp_t1_hdr->bridge_ctrl_int_pin_int_line_reg.int_pin);
    FpFwCliPrint("Bridge Control                            : 0x%lx\n", (rp_t1_hdr->bridge_ctrl_int_pin_int_line_reg.as_uint32 >> 16));
    FpFwCliPrint("============================ End ============================\n");
    /* clang-format on */
}
