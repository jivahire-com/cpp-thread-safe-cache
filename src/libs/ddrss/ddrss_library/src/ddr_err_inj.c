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
#include <cper.h> // Ensure ACPI_EINJ_INVALID_ACCESS and related macros are defined
#include <ddr_err_inj.h>
#include <ddrss.h>
#include <ddrss_lib.h>
#include <fpfw_cfg_mgr.h>
#include <health_monitor.h>
#include <kng_soc_constants.h>
#include <nvic.h>      // Has nested include of cmsis_gcc_m.h for __DSB() intrinsic
#include <ras_agent.h> // Include for ras_agent_entity_t
#include <silibs_ap_top_regs.h>
#include <silibs_platform.h>
#include <stdint.h> // Ensure uint32_t is defined
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-- Symbolic Constant Macros (defines) --*/
#define DDR_ASSERT    ASSERT_FAIL
#define UINT64_FMT(x) ((unsigned long)((x) >> 32)), (unsigned long)(x)

ddr_err_inj_syndrome_t named_syndrome[] = {DDR_ERR_INJ_SYNDROME_LIST(NAMED_SYNDROME_STRUCT_INIT)};

/*-------------- Typedefs ----------------*/

/*--------- Function Prototypes ----------*/

/*---------------- Function ---------------*/
bool ddr_ecc_err_inj_validation(uint32_t mc, uint16_t BIT)
{
    bool ret = true;
    if (mc >= DDRSS_MAX_MC_NUM)
    {
        ret = false;
    }
    if (BIT > 0x3FF)
    {
        ret = false;
    }
    return ret;
}

void ddr_err_inj_ecc_ce(uint32_t mc)
{
    ddr_ecc_error_injection(0, mc, 0x0, BIT1);
}

void ddr_err_inj_ecc_ue(uint32_t mc)
{
    ddr_ecc_error_injection(0, mc, 0x0, BIT0);
}

void ddr_ecc_error_injection(int32_t die_num, uint32_t mc, uint64_t p_addr, uint16_t Bit)
{
    silibs_status_t sts;
    ddrss_media_addr_t m_addr;
    uint64_t p_addr_new;
    uint32_t tgt_mc;
    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();

    if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP))
    {
        printf("Cannot support ecc_ue_error_injection on FPGA\n");
        return;
    }

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

// Note that CA Parity errors are fatal
int ddr_ca_parity_error_injection(uint32_t mc, DDRSS_MEDIA_CA_INJ_CMD cmd)
{
    bool valid_cmd = false;
    int sts = SILIBS_SUCCESS;
    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();

    if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP))
    {
        printf("Cannot support CA parity error injection in FPGA series platform!\n");
        return SILIBS_E_DEVICE;
    }

    // Check if cmd is one of the valid enum values from DDRSS_MEDIA_CA_INJ_CMD
    for (int i = 0; i <= DDRSS_MEDIA_CA_INJ_CMD_DRFMSB; i++)
    {
        if (cmd == i)
        {
            valid_cmd = true;
            break;
        }
    }
    if (!valid_cmd)
    {
        printf("Invalid command for CA parity error injection: 0x%X\n", cmd);
        return SILIBS_E_PARAM;
    }

    ddrss_media_ca_err_inj_info_t ca_err_inj = {0};
    ca_err_inj.cmd = cmd;
    // 5/20/2025  Maurice says cmd_count can be 0-255.  It controls how soon you will see alert signal after
    // the parity error in MC cycles. No real world visisble difference.
    ca_err_inj.cmd_count = 128;
    ca_err_inj.phase = 0;
    ca_err_inj.alert_latency = 32;

    sts = ddrss_inject_media_ca_err(mc, &ca_err_inj);
    __DSB();
    return sts;
}

acpi_einj_cmd_status_t ddr_error_injection_cb(ras_einj_info_t* einj_payload, void* ctx)
{
    FPFW_UNUSED(ctx);

    if (einj_payload == NULL)
    {
        printf("Error: Invalid EINJ payload (null)\n");
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (einj_payload->component_group == ACPI_ERROR_DOMAIN_STD_MEMORY)
    {
        printf("Please see the WIKI for ECC error injection details.\n");
        return ACPI_EINJ_INVALID_ACCESS;
    }

    if (einj_payload->component_group != ACPI_ERROR_DOMAIN_DDR)
    {
        // Only support DDR and standard memory error domains for now
        printf("Error: Unsupported component group (0x%X)for DDR error injection\n", einj_payload->component_group);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Check for potentially invalid array indexes
    if (einj_payload->param_type.error_type >= DDR_ERR_INJ_SYNDROME_COUNT)
    {
        printf("Error: Invalid error type (%d) in EINJ payload\n", einj_payload->param_type.error_type);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Check that health manager routed the correct die's error injection callback to us
    if (einj_payload->component_instance != idsw_get_die_id())
    {
        printf("Error: Component instance (%d) does not match current die ID (%d)\n",
               einj_payload->component_instance,
               idsw_get_die_id());
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Check for valid mc value
    uint32_t base_mc = (idsw_get_die_id() == DIE_1) ? DDRSS_MAX_MC_NUM_PER_DIE : 0;
    if ((einj_payload->status_operation.value < base_mc) ||
        (einj_payload->status_operation.value >= base_mc + DDRSS_MAX_MC_NUM_PER_DIE))
    {
        printf("Error: Invalid memory controller index (%d) in EINJ payload\n", einj_payload->status_operation.value);
        return ACPI_EINJ_INVALID_ACCESS;
    }

    // Call appropriate error injection function based on the error type with its function pointer
    named_syndrome[einj_payload->param_type.error_type].inj_func(einj_payload->status_operation.value);

    return ACPI_EINJ_SUCCESS;
}

// Support test case 1541196
void ddr_err_inj_media_patrol_scrub_ce(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = NULL; // Initialize to NULL
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_MREB_PATROL_SCRUB_CE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS ||
        ddrss_ras_agent == NULL)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("Patrol scrub CE injected successfully\n");
    }
}

void ddr_err_inj_fedb_merge_data_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = NULL; // Initialize to NULL
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_DATA_UE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS ||
        ddrss_ras_agent == NULL)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("FEDB merge data UE injected successfully\n");
    }
}

// Support test case 1828223
void ddr_err_inj_fedb_merge_data_ce(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = NULL; // Initialize to NULL
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_DATA_CE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS ||
        ddrss_ras_agent == NULL)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("FEDB merge data CE injected successfully\n");
    }
}

void ddr_err_inj_fedb_merge_data_parity_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = NULL; // Initialize to NULL
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_DATA_PARITY_UE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS ||
        ddrss_ras_agent == NULL)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("FEDB merge data parity UE injected successfully\n");
    }
}

void ddr_err_inj_fedb_merge_strobe_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = NULL; // Initialize to NULL
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_STROBE_UE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS ||
        ddrss_ras_agent == NULL)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("FEDB merge strobe UE injected successfully\n");
    }
}

void ddr_err_inj_fedb_merge_strobe_parity_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = NULL; // Initialize to NULL
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_STROBE_PARITY_UE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS ||
        ddrss_ras_agent == NULL)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("FEDB merge strobe parity UE injected successfully\n");
    }
}

void ddr_err_inj_fedb_strobe_array_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = NULL; // Initialize to NULL
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_STROBE_ARRAY_UE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS ||
        ddrss_ras_agent == NULL)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("FEDB strobe array UE injected successfully\n");
    }
}

// Support test case 1831500
void ddr_err_inj_media_patrol_scrub_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_MREB_PATROL_SCRUB_UE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("Media Patrol Scrub UE injected successfully\n");
    }
}

void ddr_err_inj_fecq_fedb_data_array_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_DATA_ARRAY_UE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS ||
        ddrss_ras_agent == NULL)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("FECQ FEDB DATA ARRAY UE injected successfully\n");
    }
}

// Support test case 1877177
void ddr_err_inj_mainline_traffic_ce(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_MREB_MAINLINE_TRAFFIC_CE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("Mainline traffic CE injected successfully\n");
    }
}

// Support test case 1877181
void ddr_err_inj_mrdp_parity_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;
    KNG_PLAT_ID platform_id = idsw_get_platform_sdv();

    if ((platform_id == PLATFORM_FPGA_LARGE) || (platform_id == PLATFORM_FPGA_LARGE_RVP))
    {
        printf("Cannot support MRDP parity error injection in FPGA series platform!\n");
        return;
    }

    int idx = get_syndrome_index("DDR_ERR_INJ_MRDP_PARITY_ERROR");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("MRDP Parity error injected successfully\n");
    }
}

// Support test case 2031570
void ddr_err_inj_mainline_traffic_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_MREB_MAINLINE_TRAFFIC_UE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("Mainline traffic CE injected successfully\n");
    }
}

// Support test case 2031571
void ddr_err_inj_ca_parity_persistent(uint32_t mc)
{
    ddrss_cfg_knobs_t ddrss_cfgs;

    // Use API to inject CA parity error
    ddrss_get_config(&ddrss_cfgs);

    if (ddrss_cfgs.ext_knobs.ca_parity_err_recovery_en != 1)
    {
        printf("Error: ca_parity_err_recovery_en must be 1 to support this type of error injection.  "
               "Aborting.\n");
        return;
    };
    ddr_ca_parity_error_injection(mc, DDRSS_MEDIA_CA_INJ_CMD_RD);
}

// Support test case 2093837
void ddr_err_inj_ca_parity_transient(uint32_t mc)
{
    ddrss_cfg_knobs_t ddrss_cfgs;

    // Use API to inject CA parity error
    ddrss_get_config(&ddrss_cfgs);

    if (ddrss_cfgs.ext_knobs.ca_parity_err_recovery_en != 0)
    {
        printf("Error: ca_parity_err_recovery_en must be 0 to support this type of error injection.  "
               "Aborting.\n");
        return;
    };
    ddr_ca_parity_error_injection(mc, DDRSS_MEDIA_CA_INJ_CMD_RD);
}

// Support test case TBD
void ddr_err_rh_counters_sram_parity(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_RH_COUNTERS_SRAM_PARITY");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("Row hammer counters SRAM parity error injected successfully\n");
    }
}

// Support test case TBD
void ddr_err_rh_drfm_sram_parity(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_RH_DRFM_SRAM_PARITY");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("Row hammer DRFM SRAM parity error injected successfully\n");
    }
}

void ddr_err_xts_aes_keystore_ce(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_XTS_AES_KEYSTORE_CE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("XTS AES keystore CE injected successfully\n");
    }
}

void ddr_err_xts_aes_keystore_ue(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_XTS_AES_KEYSTORE_UE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("XTS AES keystore UE injected successfully\n");
    }
}

void ddr_err_bcp_read_addr_not_in_ddr(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_BCP_READ_ADDR_NOT_IN_DDR");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("BCP read addr not in DDR error injected successfully\n");
    }
}

void ddr_err_bcp_read_blocked_by_pas(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_BCP_READ_BLOCKED_BY_PAS");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("BCP read blocked by PAS error injected successfully\n");
    }
}

void ddr_err_bcp_write_addr_not_in_ddr(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_BCP_WRITE_ADDR_NOT_IN_DDR");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("BCP write addr not in DDR error injected successfully\n");
    }
}

void ddr_err_bcp_write_blocked_by_pas(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_BCP_WRITE_BLOCKED_BY_PAS");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("BCP write blocked by PAS error injected successfully\n");
    }
}

void ddr_err_bcp_chi_unsupported_opcode(uint32_t mc)
{
    ras_agent_entity_t* ddrss_ras_agent = {0};
    uint64_t types = 0;

    int idx = get_syndrome_index("DDR_ERR_INJ_BCP_CHI_UNSUPPORTED_OPCODE");
    if (idx < 0)
    {
        printf("Error: Syndrome not found\n");
        return;
    }
    if (ddrss_get_ras_agent(mc, (DDRSS_RAS_NODE_ID)named_syndrome[idx].erg, &ddrss_ras_agent) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to get RAS agent\n");
        return;
    }
    types = named_syndrome[idx].ras_arm_err_bitmask;
    ddrss_ras_agent->syn = &(named_syndrome[idx].syndrome);

    if (ras_arm_agent_trigger_by_type(ddrss_ras_agent, types) != SILIBS_SUCCESS)
    {
        printf("Error: Failed to trigger error injection\n");
    }
    else
    {
        printf("BCP CHI unsupported opcode error injected successfully\n");
    }
}

int get_syndrome_index(const char* name)
{
    for (int i = 0; i < (int)(sizeof(named_syndrome) / sizeof(named_syndrome[0])); i++)
    {
        if (strcmp(named_syndrome[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}
