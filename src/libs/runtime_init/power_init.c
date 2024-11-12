//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file power_init.c
 * This file contains the config/initialization for the power service
 */

/*------------- Includes -----------------*/

#include "power_dfwk.h"      // for power_service_interface_t, power_...
#include "power_runconfig.h" // for VM_FLAGS_NONE, VM_FLAGS_DIV2, pow...

#include <FpFwAssert.h>            // for FPFW_RUNTIME_ASSERT
#include <atu_api.h>               // for MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR
#include <core_cluster_top_regs.h> // for CORE_CLUSTER_TOP_CORE_CLUSTER0_AD...
#include <corebits.h>
#include <fpfw_init.h> // for fpfw_init_get_handle, FPFW_INIT_S...
#include <idhw.h>
#include <idsw.h> // for idsw_get_die_id
#include <idsw_kng.h>
#include <kng_soc_constants.h> // for NUM_AP_CORES_PER_DIE
#include <platform_core_config.h>
#include <power_init.h> // for power_init, power_interface_init
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <startup_shutdown.h>
#include <stdbool.h> // for false, true
#include <stdint.h>  // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(
    pwr_svc,
    FPFW_INIT_DEPENDENCIES("dfwk", "fuse_svc", "atu_svc", "gpio_lib", "icc_d2dmbx", "avs0_int", "avs1_int", "avs2_int", "avs3_int", "hw_ver", "cfg_mgr"))
{
    static power_service_t power_service;
    static power_service_config_t power_config = {
        .soc_pvt_base = (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_PVT_ADDRESS),
        .cluster_stride = (CORE_CLUSTER_TOP_CORE_CLUSTER1_ADDRESS - CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS),
        .scp_exp_csr_base = (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCP_EXP_CSR_ADDRESS),
        .scf_mhu_base = (SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_SCF_MHU_ADDRESS),
        .scp_pwrctrl_base = (SCP_TOP_SCP_PWR_CTRL_ADDRESS),
        .soc_vm =
            {
                {.flags = VM_FLAGS_NONE}, /* Vsoc */
                {.flags = VM_FLAGS_NONE}, /* Vpciep75 */
                {.flags = VM_FLAGS_DIV2}, /* Vddq1p1 */
                {.flags = VM_FLAGS_DIV2}, /* Vpll1p2 */
                {.flags = VM_FLAGS_NONE}, /* Vsoc */
                {.flags = VM_FLAGS_NONE}, /* Vpciep75 */
                {.flags = VM_FLAGS_DIV2}, /* Vddq1p1 */
                {.flags = VM_FLAGS_DIV2}, /* Vgpio1p2 */
                {.flags = VM_FLAGS_NONE}, /* Vsoc */
                {.flags = VM_FLAGS_NONE}, /* Vd2d0p55 */
                {.flags = VM_FLAGS_NONE}, /* Vclk0p875 */
                {.flags = VM_FLAGS_NONE}, /* Vpll0p875 */
                {.flags = VM_FLAGS_DIV2}, /* Vddq1p1 */
                {.flags = VM_FLAGS_NONE}, /* Vsoc */
                {.flags = VM_FLAGS_NONE}, /* Vd2d0p55 */
                {.flags = VM_FLAGS_NONE}, /* Vclk0p875 */
                {.flags = VM_FLAGS_NONE}, /* Vpll0p875 */
                {.flags = VM_FLAGS_DIV2}, /* Vddq1p1 */
            },
        .tile_vm =
            {
                {.flags = VM_FLAGS_NONE}, /* Vcore0 */
                {.flags = VM_FLAGS_NONE}, /* Vcore1 */
                {.flags = VM_FLAGS_DIV2}, /* Vcpu0  */
                {.flags = VM_FLAGS_NONE}, /* Vsys   */
            },
        .avs_details = {{
                            .bus_id = 0,
                            .rail_id = 0,
                        }, // Die0
                        {
                            .bus_id = 0,
                            .rail_id = 1,
                        },
                        {
                            .bus_id = 1,
                            .rail_id = 0,
                        },
                        {
                            .bus_id = 1,
                            .rail_id = 1,
                        },
                        {
                            .bus_id = 2,
                            .rail_id = 0,
                        },
                        {
                            .bus_id = 2,
                            .rail_id = 1,
                        },
                        {
                            .bus_id = 3,
                            .rail_id = 0,
                        },
                        {
                            .bus_id = 3,
                            .rail_id = 1,
                        },
                        {
                            .bus_id = 0,
                            .rail_id = 0,
                        }, // Die1
                        {
                            .bus_id = 0,
                            .rail_id = 1,
                        }},
    };
    static_assert(sizeof(power_config.avs_details) / sizeof(power_avs_bus_rail_t) == MPCL_VR_COUNT,
                  "sizeof power_config.avs_details != MPCL_VR_COUNT");

    // platform defaults
    power_config.cluster_pex_base = MSCP_ATU_AP_WINDOW_CORE_CLUSTER_DIE_BASE_ADDR;
    power_config.platform_cores_in_die = &platform_cores;
    power_config.platform_die_core_count = NUM_AP_CORES_PER_DIE;
    power_config.platform_soc_power_support = false;
    power_config.platform_core_power_support = false;

    // is boot dual die?
    power_config.platform_is_multi_die = (!idhw_is_single_die_boot_en());

    // platform overrides
    switch (idsw_get_platform_sdv())
    {
    case PLATFORM_SVP_SIM:
        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1811925/
        // update based on https://azurecsi.visualstudio.com/Dev/_workitems/edit/1811919
        power_config.platform_cores_in_die = &svp_cores;
        power_config.platform_soc_power_support = true;
        power_config.platform_core_power_support = true;
        power_config.platform_is_multi_die = false;
        break;
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        power_config.platform_cores_in_die = &fpga_platform_cores;
        // FPGA r21 supports core and soc power management
        power_config.platform_soc_power_support = true;
        power_config.platform_core_power_support = true;
        break;
    case PLATFORM_EMU:
    case PLATFORM_EMU_1D:
    case PLATFORM_EMU_2D:
        power_config.platform_cores_in_die = &platform_cores;
        power_config.platform_core_power_support = true;
        break;
    case PLATFORM_EMU_1D_8C:
    case PLATFORM_EMU_2D_8C:
        power_config.platform_cores_in_die = &zebu_cores_8C_model;
        power_config.platform_core_power_support = true;
        break;
    default:
        break;
    }

    // setup necessary config for dual die
    if (power_config.platform_is_multi_die)
    {
        power_config.icc_d2d_ctx = fpfw_init_get_handle("icc_d2dmbx");
    }

    power_config.scp_avs_insts[AVS_BUS0] = (scp_avs_interface_t*)fpfw_init_get_handle("avs0_int");
    if (idsw_get_die_id() == DIE_0) // DIE_1 only has one AVSBus.
    {
        power_config.scp_avs_insts[AVS_BUS1] = (scp_avs_interface_t*)fpfw_init_get_handle("avs1_int");
        power_config.scp_avs_insts[AVS_BUS2] = (scp_avs_interface_t*)fpfw_init_get_handle("avs2_int");
        power_config.scp_avs_insts[AVS_BUS3] = (scp_avs_interface_t*)fpfw_init_get_handle("avs3_int");
        power_config.vr_idx_info.flattened_vr_start_idx = MPCL_VR_VCPU;
        power_config.vr_idx_info.vcpu_idx = MPCL_VR_VCPU;
        power_config.vr_idx_info.vsys_idx = MPCL_VR_VSYS; // VSYS is on AVSBus1, rail 0, so index 2 since 2 rails per AVSBus.
        power_config.num_vr = (MPCL_VR_VCPU1 - MPCL_VR_VCPU); // 8 VR's on Die0
    }
    else
    {
        power_config.vr_idx_info.flattened_vr_start_idx = MPCL_VR_VCPU1;
        power_config.vr_idx_info.vcpu_idx = MPCL_VR_VCPU1;
        power_config.vr_idx_info.vsys_idx = NO_VSYS;           // No VSYS on Die1
        power_config.num_vr = (MPCL_VR_COUNT - MPCL_VR_VCPU1); // 2 VR's on Die1;
    }
    power_init(&power_service, fpfw_init_get_handle((void*)"dfwk"), &power_config);
    pwr_avs_initialize(power_config.scp_avs_insts);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &power_service};
}

FPFW_INIT_COMPONENT(pwr_int, FPFW_INIT_DEPENDENCIES("pwr_svc", "sos_int"))
{
    static power_service_interface_t power_interface;
    power_interface_init(fpfw_init_get_handle("pwr_svc"), &power_interface);

    /*========= Begin code for SSI registration =========*/
    // create an interface specifically for SSI and register it
    static power_service_interface_t power_ssi_interface;
    power_interface_init(fpfw_init_get_handle("pwr_svc"), &power_ssi_interface);

    // static data for SSI registration
    static startup_ssi_registration_t ssi_registration;
    int32_t status = sos_register_ssi(fpfw_init_get_handle("sos_int"), &ssi_registration, &power_ssi_interface.header);
    FPFW_RUNTIME_ASSERT(status == FPFW_INIT_STATUS_SUCCESS);
    /*=========== End code for SSI registration ==========*/

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &power_interface};
}