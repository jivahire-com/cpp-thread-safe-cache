//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 *  @file tower.c
 *  This modules configures various SOC towers as done by the SCP FW
 */

/*------------- Includes -----------------*/

#include <accelerator_ip.h>
#include <atu_lib.h> // for atu_map, atu_unmap, atu_map_entry_t
#include <bug_check.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_status.h>
#include <hsp_firmware_headers.h>
#include <idsw.h>              // for idsw_get_platform_sdv
#include <idsw_kng.h>          // for PLATFORM_SVP_SIM, KNG_PLAT_ID, PLATFO...
#include <kng_atu_mappings.h>  // for ATU_MAPPING_D0_RPSS0_TOWER, ATU_MAPPI...
#include <kng_soc_constants.h> // for SOC_D0, SOC_D1, D0_VAB1_RPSS1, D0_VAB...
#include <silibs_platform.h>   // for ASSERT_FAIL
#include <stdbool.h>           // for true, false
#include <stdint.h>            // for uint16_t, uint8_t, uintptr_t
#include <stdio.h>             // for printf
#include <system_info.h>
#include <tower.h>          // for tower_init
#include <tower_sequence.h> // for tower_sequence_soc_init_params_t, tow....

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
#define TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SVP                                                  \
    ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3) | \
     (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS))
#define TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_SVP                                                  \
    ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3) | \
     (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS))

#define TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SVP_MIN_CONFIG ((1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS))

#define TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_FPGA \
    ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS))
#define TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_FPGA \
    ((1 << D1_VAB1_RPSS1) | (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS))

#define TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SILICON                                              \
    ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3) | \
     (1 << D0_VAB4_SDMSS) | (1 << D0_VAB5_CDEDSS_IOSS))
#define TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_SILICON                                              \
    ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3) | \
     (1 << D1_VAB4_SDMSS) | (1 << D1_VAB5_CDEDSS_IOSS))

static uint16_t tower_vab_instances_to_be_enabled(uint8_t die_num)
{
    BUG_ASSERT_PARAM(die_num < NUM_DIE, die_num, 0);
    uint16_t vab_instances_to_init = 0;
    KNG_PLAT_ID plat = idsw_get_platform_sdv();
    switch (plat)
    {
    case PLATFORM_SVP_SIM:
        vab_instances_to_init =
            (die_num == 0) ? TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SVP : TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_SVP;
        break;

    case PLATFORM_SVP_MIN_CONFIG_SIM:
        /* SVP min config is only a single die configuration with SDM/CDED VABs */
        vab_instances_to_init = (die_num == 0) ? TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SVP_MIN_CONFIG : 0x00;
        break;

    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        vab_instances_to_init = (die_num == 0) ? TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_FPGA
                                               : TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_FPGA;
        break;

    case PLATFORM_RVP_EVT_SILICON:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_2D_8C:
        vab_instances_to_init = (die_num == 0) ? TOWER_DIE0_VAB_INSTANCES_ENABLED_ON_SILICON
                                               : TOWER_DIE1_VAB_INSTANCES_ENABLED_ON_SILICON;
        break;

    default:
        printf("not supported platform id: %d\n", plat);
        break;
    }

    return vab_instances_to_init;
}

#define TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_SVP \
    ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3))
#define TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_SVP \
    ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3))

#define TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_FPGA ((1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2))
#define TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_FPGA ((1 << D1_VAB1_RPSS1))

#define TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_SILICON \
    ((1 << D0_VAB0_RPSS0) | (1 << D0_VAB1_RPSS1) | (1 << D0_VAB2_RPSS2) | (1 << D0_VAB3_RPSS3))
#define TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_SILICON \
    ((1 << D1_VAB0_RPSS0) | (1 << D1_VAB1_RPSS1) | (1 << D1_VAB2_RPSS2) | (1 << D1_VAB3_RPSS3))

static uint16_t tower_rpss_instances_to_be_enabled(uint8_t die_num)
{
    BUG_ASSERT_PARAM(die_num < NUM_DIE, die_num, 0);
    uint16_t rpss_instances_to_init = 0;
    KNG_PLAT_ID plat = idsw_get_platform_sdv();
    switch (plat)
    {
    case PLATFORM_SVP_SIM:
        rpss_instances_to_init = (die_num == 0) ? TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_SVP
                                                : TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_SVP;
        break;

    case PLATFORM_SVP_MIN_CONFIG_SIM:
        /* SVP min config does not have any PCIe root port subsystems */
        rpss_instances_to_init = 0x00;
        break;

    case PLATFORM_FPGA:
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        rpss_instances_to_init = (die_num == 0) ? TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_FPGA
                                                : TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_FPGA;
        break;

    case PLATFORM_RVP_EVT_SILICON:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D:
    case PLATFORM_EMU_2D_8C:
        rpss_instances_to_init = (die_num == 0) ? TOWER_DIE0_RPSS_INSTANCES_ENABLED_ON_SILICON
                                                : TOWER_DIE1_RPSS_INSTANCES_ENABLED_ON_SILICON;
        break;

    default:
        printf("not supported platform id: %d\n", plat);
        break;
    }

    return rpss_instances_to_init;
}

// Map the VAB-SS instead of just the tower space because this sequence also configures SMMU straps and
// SMMU reset de-assertion to allow configuring the VAB FMU
static atu_map_entry_t atu_vab_maps[MAX_VAB_INSTANCES] = {
    ATU_MAPPING_D0_VAB0_RPSS0(),
    ATU_MAPPING_D0_VAB1_RPSS1(),
    ATU_MAPPING_D0_VAB2_RPSS2(),
    ATU_MAPPING_D0_VAB3_RPSS3(),
    ATU_MAPPING_D1_VAB0_RPSS0(),
    ATU_MAPPING_D1_VAB1_RPSS1(),
    ATU_MAPPING_D1_VAB2_RPSS2(),
    ATU_MAPPING_D1_VAB3_RPSS3(),
    ATU_MAPPING_D0_VAB4_SDMSS(),
    ATU_MAPPING_D1_VAB4_SDMSS(),
    ATU_MAPPING_D0_VAB5_CDEDSS_IOSS(),
    ATU_MAPPING_D1_VAB5_CDEDSS_IOSS(),
};

static atu_map_entry_t atu_rpss_tower_maps[NUM_RPSS] = {
    ATU_MAPPING_D0_RPSS0_TOWER(),
    ATU_MAPPING_D0_RPSS1_TOWER(),
    ATU_MAPPING_D0_RPSS2_TOWER(),
    ATU_MAPPING_D0_RPSS3_TOWER(),
    ATU_MAPPING_D1_RPSS0_TOWER(),
    ATU_MAPPING_D1_RPSS1_TOWER(),
    ATU_MAPPING_D1_RPSS2_TOWER(),
    ATU_MAPPING_D1_RPSS3_TOWER(),
};

/**
 * @brief Post SCP init tower enable
 *
 * \b Description:
 * This function informs HSP to enable the DDRSS and CDEDSS towers, the flags in the message
 * is used to inform HSP on whether isolation is enabled or not
 *
 * @param[in] icc_ctx - Pointer to icc base context structure
 * @param[in] isolation_flag - indicates if accelerator isolation is enabled (0x0) or not (0x4)
 *
 * @retval void
 */
static void hsp_send_recv_post_scp_init_tower_config(fpfw_icc_base_ctx_t* icc_ctx, uint8_t isolation_flag)
{
    tower_sequence_knobs_t tower_cfg = config_get_tower_knobs().tower_mode_cfg;
    size_t recv_msg_size_bytes = 0;
    kng_hsp_mailbox_msg msg = {};
    msg.header.cmd = HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_REQ;
    msg.header.flags = isolation_flag;
    msg.tower_config_req.tower_unlock_flags.tower_configure_all_apus_as_nonsecure =
        tower_cfg.tower_configure_all_apus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_configure_ddrss_apus_as_nonsecure =
        tower_cfg.tower_configure_ddrss_apus_as_nonsecure;
    msg.tower_config_req.tower_unlock_flags.tower_configure_all_pmus_as_nonsecure =
        tower_cfg.tower_configure_all_pmus_as_nonsecure;
    // HSP doesn't configure RPSS or VAB, no need to send those knobs
    msg.tower_config_req.tower_unlock_flags.tower_rpss_os_first_ras_err_handling = 0;
    msg.tower_config_req.tower_unlock_flags.tower_vab_os_first_ras_err_handling = 0;

    printf("Prepare to send %x to HSP\n", msg.header.cmd);
    //! Send the message to HSP & get response, blocking call
    fpfw_status_t icc_status =
        fpfw_icc_base_send_recv_sync(icc_ctx, &msg, sizeof(kng_hsp_mailbox_msg), &recv_msg_size_bytes);
    printf("%x received from HSP\n", HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_RSP);

    //! Verify sync return status & response message
    BUG_ASSERT(icc_status == FPFW_ICC_BASE_STATUS_SUCCESS);
    BUG_ASSERT(recv_msg_size_bytes > 0);
    BUG_ASSERT(msg.header.cmd == HSP_MAILBOX_CMD_POST_SCP_INIT_TOWER_CONFIG_RSP);
    BUG_ASSERT(msg.rsp.status == HSP_MAILBOX_RSP_STATUS_SUCCESS);
}

void tower_init(uint8_t die_num, fpfw_icc_base_ctx_t* icc_ctx)
{
    BUG_ASSERT_PARAM(die_num < NUM_DIE, die_num, 0);

    bool is_warm_start = system_info_is_warm_start();

    tower_sequence_soc_init_params_t tower_sequence_params = {0};

    tower_sequence_knobs_t tower_cfg = config_get_tower_knobs().tower_mode_cfg;

    tower_sequence_params.tower_cfg = &tower_cfg;

    // Always set up MSI attributes for VAB SMMUs (so that they are delivered to cores correctly)
    tower_sequence_params.vab_configure_msi_attr = true;

    // Fabric Tower
    tower_sequence_params.tower_configure_fabric_apu = !is_warm_start;
    tower_sequence_params.tower_configure_fabric_fmu = true;
    atu_map_entry_t atu_fabric_tower_map = ATU_MAPPING_FABRIC_TOWER((die_num == 0 ? SOC_D0 : SOC_D1));
    BUG_ASSERT(!atu_map(ATU_ID_MSCP, &atu_fabric_tower_map));
    tower_sequence_params.tower_fabric_tower_resolved_addr = atu_fabric_tower_map.mscp_start_address;

    // Peripheral Tower
    tower_sequence_params.tower_configure_periph_apu = !is_warm_start;
    tower_sequence_params.tower_configure_periph_fmu = true;
    atu_map_entry_t atu_peripheral_tower_map = ATU_MAPPING_PERIPHERAL_TOWER((die_num == 0 ? SOC_D0 : SOC_D1));
    BUG_ASSERT(!atu_map(ATU_ID_MSCP, &atu_peripheral_tower_map));
    tower_sequence_params.tower_periph_tower_resolved_addr = atu_peripheral_tower_map.mscp_start_address;

    // D2DSS towers
    atu_map_entry_t atu_d2d_cfg0_tower_map = ATU_MAPPING_D2DSS_CFG0_TOWER((die_num == 0 ? SOC_D0 : SOC_D1));
    atu_map_entry_t atu_d2d_cfg1_tower_map = ATU_MAPPING_D2DSS_CFG1_TOWER((die_num == 0 ? SOC_D0 : SOC_D1));
    ASSERT_FAIL(!atu_map(ATU_ID_MSCP, &atu_d2d_cfg0_tower_map));
    tower_sequence_params.tower_d2dss_cfg0_tower_resolved_addr = atu_d2d_cfg0_tower_map.mscp_start_address;

    ASSERT_FAIL(!atu_map(ATU_ID_MSCP, &atu_d2d_cfg1_tower_map));
    tower_sequence_params.tower_d2dss_cfg1_tower_resolved_addr = atu_d2d_cfg1_tower_map.mscp_start_address;

    tower_sequence_params.tower_configure_d2dss_apu = !is_warm_start;
    tower_sequence_params.tower_configure_d2dss_fmu = true;

    // VAB Towers
    uint16_t vab_instances_to_init = tower_vab_instances_to_be_enabled(die_num);
    tower_sequence_params.tower_vab_instances_enabled = vab_instances_to_init;
    printf("Mask of VAB instances to be enabled: 0x%x\n", vab_instances_to_init);

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vab_instances_to_init & (0x1 << vab_id)) != 0)
        {
            printf("Configure VAB ATU map for VAB: 0x%x\n", vab_id);
            BUG_ASSERT(!atu_map(ATU_ID_MSCP, &atu_vab_maps[vab_id]));
            tower_sequence_params.tower_vab_resolved_addr[vab_id] = atu_vab_maps[vab_id].mscp_start_address;
            tower_sequence_params.vab_smmu_l0gpt_size[vab_id] = 0;
            // GPT enable / disable would be a knob controlled by TF-A. This param here doesn't enable GPT
            // but configures the GPT size (needs to be programmed before smmu is released out of reset) which
            // can be done regardless of whether GPT/GPC is enabled or not.
            tower_sequence_params.vab_smmu_gpt_enabled[vab_id] = true;
        }
    }
    if (vab_instances_to_init != 0)
    {
        tower_sequence_params.tower_configure_vab_sam = !is_warm_start;
        tower_sequence_params.tower_configure_vab_apu = !is_warm_start;
        tower_sequence_params.tower_configure_vab_fmu = true;
    }

    // RPSS towers
    uint8_t rpss_instances_to_init = tower_rpss_instances_to_be_enabled(die_num);
    tower_sequence_params.tower_rpss_instances_enabled = rpss_instances_to_init;
    printf("Mask of RPSS instances to be enabled: 0x%x\n", rpss_instances_to_init);

    for (uint8_t rpss_id = 0; rpss_id < NUM_RPSS; rpss_id++)
    {
        if ((rpss_instances_to_init & (0x1 << rpss_id)) != 0)
        {
            printf("Configure RPSS Tower ATU map for RPSS: 0x%x\n", rpss_id);
            BUG_ASSERT(!atu_map(ATU_ID_MSCP, &atu_rpss_tower_maps[rpss_id]));
            tower_sequence_params.tower_rpss_tower_resolved_addr[rpss_id] = atu_rpss_tower_maps[rpss_id].mscp_start_address;
        }
    }
    if (rpss_instances_to_init != 0)
    {
        tower_sequence_params.tower_configure_rpss_sam = !is_warm_start;
        tower_sequence_params.tower_configure_rpss_apu = !is_warm_start;
        tower_sequence_params.tower_configure_rpss_fmu = true;
    }

    // SDMSS tower
    printf("Configure SDMSS Tower ATU map\n");
    atu_map_entry_t sdmss_tower_map = ATU_MAPPING_SDMSS_TOWER((die_num == 0 ? D0_SDMSS : D1_SDMSS));
    BUG_ASSERT(!atu_map(ATU_ID_MSCP, &sdmss_tower_map));
    tower_sequence_params.tower_sdmss_tower_resolved_addr = sdmss_tower_map.mscp_start_address;
    tower_sequence_params.tower_configure_sdmss_sam = !is_warm_start;
    tower_sequence_params.tower_configure_sdmss_apu = !is_warm_start;
    tower_sequence_params.tower_configure_sdmss_fmu = true;

    if (accel_is_isolation_enabled(ACCEL_ID_SDM))
    {
        printf("Configure SDMSS tower for isolation enabled\n");
        tower_sequence_params.tower_sdmss_isolation_enabled = true;
    }
    else
    {
        printf("Configure SDMSS tower for isolation disabled\n");
        tower_sequence_params.tower_sdmss_isolation_enabled = false;
    }

    // CDEDSS Tower
    // Send a message to HSP for configuring CDEDSS tower (along with isolation enabled/disabled)
    if (system_info_is_hsp_present())
    {
        if (accel_is_isolation_enabled(ACCEL_ID_CDED))
        {
            printf("Configure CDEDSS tower for isolation enabled\n");
            hsp_send_recv_post_scp_init_tower_config(icc_ctx, 0);
        }
        else
        {
            printf("Configure CDEDSS tower for isolation disabled\n");
            hsp_send_recv_post_scp_init_tower_config(icc_ctx, HSP_MAILBOX_FLAGS_ACCL_ISOLATION_DISABLED);
        }

        // Not sure if we need to add more variables to the tower struct and keep track of it
    }

    // IOSS tower
    printf("Configure IOSS tower ATU map\n");
    atu_map_entry_t ioss_tower_map = ATU_MAPPING_IOSS_TOWER((die_num == 0 ? D0_IOSS : D1_IOSS));
    BUG_ASSERT(!atu_map(ATU_ID_MSCP, &ioss_tower_map));
    tower_sequence_params.tower_ioss_tower_resolved_addr = ioss_tower_map.mscp_start_address;
    tower_sequence_params.tower_configure_ioss_sam = !is_warm_start;
    tower_sequence_params.tower_configure_ioss_apu = !is_warm_start;
    tower_sequence_params.tower_configure_ioss_fmu = true;

    tower_sequence_params.die_id = die_num;

    // TODO: WI 1869184 FMU is not supported in SVP currently
    if (IS_PLATFORM_SVP())
    {
        tower_sequence_params.tower_configure_d2dss_fmu = false;
        tower_sequence_params.tower_configure_fabric_fmu = false;
        tower_sequence_params.tower_configure_periph_fmu = false;
        tower_sequence_params.tower_configure_vab_fmu = false;
        tower_sequence_params.tower_configure_rpss_fmu = false;
        tower_sequence_params.tower_configure_sdmss_fmu = false;
        tower_sequence_params.tower_configure_ioss_fmu = false;
    }

    printf("Configure all towers\n");
    BUG_ASSERT(!tower_sequence_configure_towers(&tower_sequence_params));
    printf("Towers configured\n");

    // Un-map towers
    printf("Unmap towers\n");
    BUG_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_peripheral_tower_map));
    BUG_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_fabric_tower_map));

    if (!(IS_PLATFORM_SVP()))
    {
        BUG_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_d2d_cfg0_tower_map));
        BUG_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_d2d_cfg1_tower_map));
    }

    for (uint16_t vab_id = 0; vab_id < MAX_VAB_INSTANCES; vab_id++)
    {
        if ((vab_instances_to_init & (0x1 << vab_id)) != 0)
        {
            BUG_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_vab_maps[vab_id]));
        }
    }
    for (uint8_t rpss_id = 0; rpss_id < NUM_RPSS; rpss_id++)
    {
        if ((rpss_instances_to_init & (0x1 << rpss_id)) != 0)
        {
            BUG_ASSERT(!atu_unmap(ATU_ID_MSCP, &atu_rpss_tower_maps[rpss_id]));
        }
    }
    BUG_ASSERT(!atu_unmap(ATU_ID_MSCP, &sdmss_tower_map));
    BUG_ASSERT(!atu_unmap(ATU_ID_MSCP, &ioss_tower_map));
    printf("Towers unmapped\n");
}
