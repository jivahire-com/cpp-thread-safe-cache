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
#include <FpFwUtils.h>             // for knowing how big 1k is
#include <atu_lib.h>               // for atu_map_entry_t, atu_entry_attr_t
#include <core_cluster_top_regs.h> // for CORE_CLUSTER_TOP_CORE_CLUSTER0_AD...
#include <fpfw_init.h>             // for fpfw_init_get_handle, FPFW_INIT_S...
#include <idsw.h>                  // for idsw_get_die_id
#include <power_init.h>            // for power_init, power_interface_init
#include <silibs_ap_top_regs.h>    // for AP_TOP_D0_CORE_CLUSTER_SIZE, AP_T...
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <startup_shutdown.h>
#include <stdint.h> // for uint32_t

/*-- Symbolic Constant Macros (defines) --*/
/* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1820413
   Replace/update once ATU static mapping is available */
#define AP_CORE_CLUSTER_BASE_ADDR(die_num) \
    (((die_num) == 0) ? AP_TOP_D0_CORE_CLUSTER_ADDRESS : AP_TOP_D1_CORE_CLUSTER_ADDRESS)
#define AP_CLUSTER_BASE_ADDR(die_num, cluster_num) \
    (AP_CORE_CLUSTER_BASE_ADDR(die_num) + ((cluster_num) * AP_CLUSTER_SIZE))
// define a few macros to handle 8KB alignment
#define ATU_ALIGNMENT_SIZE (8 * FPFW_KB)
#define ATU_ALIGN(size)    (FPFW_ALIGN_BY(ATU_ALIGNMENT_SIZE, size))

/*-------------- Functions ---------------*/

FPFW_INIT_COMPONENT(pwr_svc, FPFW_INIT_DEPENDENCIES("dfwk", "fuse_svc"))
{
    const atu_entry_attr_t atu_root_attr = {ATU_BUS_ATTR_PRIV, ATU_BUS_ATTR_ROOT};
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
    };

    /* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1820413
             Remove dynamic mapping once ATU static mapping is available */

    /* == BEGIN ATU MAP of CORE_CLUSTER == */
    uint32_t die_num = idsw_get_die_id();
    atu_map_entry_t atu_map_struct = {};
    // Step-1 Setup ATU map for the entire core cluster
    atu_map_struct.ap_base_address = AP_CORE_CLUSTER_BASE_ADDR(die_num);
    atu_map_struct.mscp_start_address = 0;
    // The size of the mapped region must be 8KB aligned
    uint32_t core_cluster_size = (die_num == 0) ? AP_TOP_D0_CORE_CLUSTER_SIZE : AP_TOP_D1_CORE_CLUSTER_SIZE;
    atu_map_struct.mscp_end_address = ATU_ALIGN(core_cluster_size) - 1;
    // Confer root level privileges
    atu_map_struct.attribute.as_uint32 = atu_root_attr.as_uint32;
    FPFW_RUNTIME_ASSERT(!atu_map(ATU_ID_MSCP, &atu_map_struct));
    /* == END ATU MAP of CORE_CLUSTER == */

    power_config.cluster_pex_base = atu_map_struct.mscp_start_address;

    power_init(&power_service, fpfw_init_get_handle((void*)"dfwk"), &power_config);

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