//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_mesh.cpp
 * mesh tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for expect_value, check_expected, CmockaWra...

extern "C" {
#include <FpFwUtils.h>            // for FPFW_UNUSED
#include <cmn800_sequence.h>      // for cmn800_sequence_params_t
#include <cmn_config.h>           // for CMN800_CONFIG_CONFIG
#include <fpfw_icc_base.h>             // for fpfw_icc_base_ctx_t
#include <fpfw_status.h>
#include <hsp_firmware_headers.h> // for kng_hsp_mailbox_msg
#include <idsw.h>                 // for idsw_set_platform_sdv, PLATFORM_UN...
#include <idsw_kng.h>             // for KNG_DIE_ID, _KNG_PLAT_ID
#include <mesh.h>                 // for mesh_init
#include <stdint.h>               // for int32_t, uint32_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
bool simulate_single_die = true;
extern bool dual_die_boot; // ADO: 1825901 Deprecate after ADO is implemented by SVP
static fpfw_icc_base_ctx_t *test_icc_base_hsp_mbx_ctx;

/*------------- Functions ----------------*/
//! Mocks for mailbox primitives called inside hsp_send_recv_enable_smmu()
fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t *icc_ctx, void *payload_buffer, size_t buffer_size, size_t *output_recv_bytes)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(buffer_size);

    kng_hsp_mailbox_msg* msg = (kng_hsp_mailbox_msg*)payload_buffer;
    msg->header.cmd = HSP_MAILBOX_CMD_ENABLE_SMMU_ACCESS_RSP;
    msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
    *output_recv_bytes = sizeof(kng_hsp_mailbox_msg);

    return FPFW_ICC_BASE_STATUS_SUCCESS;
}

bool __wrap_system_info_is_hsp_present()
{
    return true;
}

//
// Setup and Teardown Functions
//
static int setup_svp_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    simulate_single_die = true;
    return 0;
}

static int setup_svp_platform_dual_die(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_SVP_SIM);
    simulate_single_die = false;
    dual_die_boot = true; // ADO: 1825901 Deprecate after ADO is implemented by SVP
    return 0;
}

static int setup_fpga_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    simulate_single_die = true;
    return 0;
}

static int setup_fpga_platform_dual_die(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_FPGA_LARGE);
    simulate_single_die = false;
    return 0;
}

static int setup_undefined_platform(void** pContext)
{
    FPFW_UNUSED(pContext);
    idsw_set_platform_sdv(PLATFORM_UNDEFINED);
    simulate_single_die = true;
    return 0;
}

//
// Mocks
//
int __wrap_cmn800_sequence(cmn800_sequence_params_t cmn800_sequence_param)
{
    check_expected(cmn800_sequence_param.die_num);
    check_expected(cmn800_sequence_param.cmn_config_enum);
    check_expected(cmn800_sequence_param.CMN_INIT);
    check_expected(cmn800_sequence_param.BOOT_2D_ENABLE);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE0);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE1);
    check_expected(cmn800_sequence_param.D2D_INIT);
    function_called();
    return 0;
}

int __wrap_d2dss_sequence(cmn800_sequence_params_t cmn800_sequence_param)
{
    check_expected(cmn800_sequence_param.die_num);
    check_expected(cmn800_sequence_param.cmn_config_enum);
    check_expected(cmn800_sequence_param.CMN_INIT);
    check_expected(cmn800_sequence_param.BOOT_2D_ENABLE);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE0);
    check_expected(cmn800_sequence_param.HNS_SPARE_DIE1);
    check_expected(cmn800_sequence_param.D2D_INIT);
    function_called();
    return 0;
}

int __wrap_cmn800_sequence_svp_updates(cmn800_sequence_params_t cmn800_sequence_param)
{
    check_expected(cmn800_sequence_param.die_num);

    function_called();
    return 0;
}
bool __wrap_idhw_is_single_die_boot_en(void)
{
    return simulate_single_die;
}

//
// Tests
//
// Single Die Boot
TEST_FUNCTION(test_mesh_init_single_die_boot_Die_0_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;

    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_single_die_boot_Die_1_SVP, setup_svp_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;

    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_single_die_boot_Die_0_FPGA, setup_fpga_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;

    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_single_die_boot_Die_1_FPGA, setup_fpga_platform, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;

    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_1D_UMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

// Dual Die Boot
TEST_FUNCTION(test_mesh_init_dual_die_boot_Die_0_SVP, setup_svp_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;

    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_d2dss_sequence);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_dual_die_boot_Die_1_SVP, setup_svp_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;

    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_64HNS_HIER_3SN_enum);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_d2dss_sequence);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_dual_die_boot_Die_0_FPGA, setup_fpga_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)0;

    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_d2dss_sequence);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}

TEST_FUNCTION(test_mesh_init_dual_die_boot_Die_1_FPGA, setup_fpga_platform_dual_die, setup_undefined_platform)
{
    // Set up expectations
    const auto test_die = (KNG_DIE_ID)1;

    expect_value(__wrap_cmn800_sequence_svp_updates, cmn800_sequence_param.die_num, test_die);
    expect_function_call(__wrap_cmn800_sequence_svp_updates);

    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_cmn800_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_cmn800_sequence);

    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.die_num, test_die);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.cmn_config_enum, CONFIG_2D_NUMA_8HNS_HIER_3SN_enum);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.CMN_INIT, 2);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.BOOT_2D_ENABLE, true);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE0, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.HNS_SPARE_DIE1, 0);
    expect_value(__wrap_d2dss_sequence, cmn800_sequence_param.D2D_INIT, 0);
    expect_function_call(__wrap_d2dss_sequence);

    // Call API under test
    mesh_init(test_die, test_icc_base_hsp_mbx_ctx);
}
}
