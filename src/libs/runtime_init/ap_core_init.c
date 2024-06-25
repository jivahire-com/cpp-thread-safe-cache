//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file ap_core_init.c
 * This file contains the config/initialization for the APcore service
 */

/*------------- Includes -----------------*/

#include <FpFwAssert.h> // for FPFW_RUNTIME_ASSERT
#include <FpFwUtils.h>  // for knowing how big 1k is
#include <ap_core_init.h>
#include <atu_lib.h>               // for atu_map_entry_t, atu_entry_attr_t
#include <core_cluster_top_regs.h> // for CORE_CLUSTER_TOP_CORE_CLUSTER0_AD...
#include <corebits.h>
#include <fpfw_init.h>          // for fpfw_init_get_handle, FPFW_INIT_S...
#include <idsw.h>               // for idsw_get_die_id
#include <idsw_kng.h>           // for NUM_DIE
#include <kng_soc_constants.h>  // for NUM_AP_CORES_PER_DIE
#include <silibs_ap_top_regs.h> // for AP_TOP_D0_CORE_CLUSTER_SIZE, AP_T...
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

#define DEFAULT_BOOT_CORE_RVBARADDR (0x00010000ull)

/*-------------- Functions ---------------*/
static void setup_ap_loop_to_self(uint64_t rvbaraddr)
{
#define ARM64_LOOP 0x14000000
    // setup loop to self
    // find SCP address for the rvbar
    uint32_t translated_rvbar;
    // expect that there is a global translation for rvbar
    FPFW_RUNTIME_ASSERT(!atu_translate_address(ATU_ID_MSCP, rvbaraddr, &translated_rvbar));
    // write a loop to self instruction at the entry point to enable side-load of FW when HSP not present, etc
    *(volatile uint32_t*)translated_rvbar = ARM64_LOOP;
}

// dependency on pwr_svc only due to https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1820413; pwr_svc already mapping, so rely on that here
FPFW_INIT_COMPONENT(ap_core_svc, FPFW_INIT_DEPENDENCIES("dfwk", "pwr_svc", "tower_cfg"))
{
#define SVP_NUM_CORES_PER_DIE 4
    // fpga platform has an unusual set of available cores
    static const corebits_t fpga_platform_cores = (corebits_t)COREBITS_INIT_UINT32(0x000c0300, 0x00c03000, 0);
    static const corebits_t platform_cores = (corebits_t)COREBITS_INIT_UINT32(0xFFFFFFFF, 0xFFFFFFFF, 0xF);

    static ap_core_service_t ap_core_service;
    static ap_core_service_config_t ap_core_config = {
        .boot_core_rvbaraddr = DEFAULT_BOOT_CORE_RVBARADDR,
        .cluster_stride = (CORE_CLUSTER_TOP_CORE_CLUSTER1_ADDRESS - CORE_CLUSTER_TOP_CORE_CLUSTER0_ADDRESS),
    };

    /* TODO: https://dev.azure.com/AzureCSI/Dev/_workitems/edit/1820413
             Update once ATU static mapping is available */

    /* == BEGIN ATU MAP of CORE_CLUSTER == */
    uint32_t die_num = idsw_get_die_id();
    atu_map_entry_t atu_map_struct = {};
    // Step-1 Setup ATU map for the entire core cluster
    atu_map_struct.ap_base_address = AP_CORE_CLUSTER_BASE_ADDR(die_num);
    atu_map_struct.mscp_start_address = 0;
    // The size of the mapped region must be 8KB aligned
    uint32_t core_cluster_size = (die_num == 0) ? AP_TOP_D0_CORE_CLUSTER_SIZE : AP_TOP_D1_CORE_CLUSTER_SIZE;
    atu_map_struct.mscp_end_address = ATU_ALIGN(core_cluster_size) - 1;
    // using find map, expecting existing mapping
    FPFW_RUNTIME_ASSERT(!atu_find_map(ATU_ID_MSCP, &atu_map_struct));
    /* == END ATU MAP of CORE_CLUSTER == */

    ap_core_config.cluster_pex_base = atu_map_struct.mscp_start_address;

    // platform defaults
    ap_core_config.platform_cores_in_die = &platform_cores;
    ap_core_config.platform_die_core_count = NUM_AP_CORES_PER_DIE;

    // platform overrides
    switch (idsw_get_platform_sdv())
    {
    case PLATFORM_SVP_SIM:
        // TODO: https://azurecsi.visualstudio.com/Dev/_workitems/edit/1811925/
        // update based on https://azurecsi.visualstudio.com/Dev/_workitems/edit/1811919
        ap_core_config.platform_die_core_count = SVP_NUM_CORES_PER_DIE;
        break;
    case PLATFORM_FPGA_LARGE:
    case PLATFORM_FPGA_LARGE_RVP:
        ap_core_config.platform_cores_in_die = &fpga_platform_cores;
        // put a loop to self at the rbvaraddr (FW load would replace this)
        setup_ap_loop_to_self(ap_core_config.boot_core_rvbaraddr);
        break;
    default:
        break;
    }
    ap_core_init(&ap_core_service, fpfw_init_get_handle((void*)"dfwk"), &ap_core_config);

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &ap_core_service};
}

FPFW_INIT_COMPONENT(ap_core_int, FPFW_INIT_DEPENDENCIES("ap_core_svc", "sos_int"))
{
    static ap_core_interface_t ap_core_interface;
    ap_core_interface_init(fpfw_init_get_handle("ap_core_svc"), &ap_core_interface);

    /*========= Begin code for SSI registration =========*/
    // create an interface specifically for SSI and register it
    static ap_core_interface_t ap_core_ssi_interface;
    ap_core_interface_init(fpfw_init_get_handle("ap_core_svc"), &ap_core_ssi_interface);
    // static data for SSI registration
    static startup_ssi_registration_t ssi_registration;
    int32_t status =
        sos_register_ssi(fpfw_init_get_handle("sos_int"), &ssi_registration, &ap_core_ssi_interface.header);
    FPFW_RUNTIME_ASSERT(status == FPFW_INIT_STATUS_SUCCESS);
    /*=========== End code for SSI registration ==========*/

    return (fpfw_init_result_t){FPFW_INIT_STATUS_SUCCESS, &ap_core_interface};
}