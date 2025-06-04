//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cli_ddr.cpp
 * Tests the init of cli ddr component
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for assert_non_null, CmockaWrapperTest, TEST_...

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED, FPFW_ARRAY_SIZE
#include <ddr_err_inj.h>
#include <ddrmctop_regs.h>
#include <ddrss_lib.h>
#include <kng_soc_constants.h>
#include <ras_common.h>
#include <silibs_ap_top_regs.h>
#include <silibs_platform.h>
#include <stddef.h> // for NULL, size_t

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
extern bool g_should_check_ras_agent_entity_id;
extern bool g_should_wrap_ddrss_get_config;
extern bool g_should_wrap_ddrss_inject_media_ca_err;

//
// Mocks
//

/*------------- Functions ----------------*/

//
// Tests
//

TEST_FUNCTION(test_ecc_ce_error_injection, NULL, NULL)
{
    // Set up pre-conditions / expectations
    const uint32_t p_addr = 0x0;
    const uint16_t Bit_Value = BIT1;
    const uint32_t expected_mc = 0;
    const int32_t expected_die_num = 0;
    ddrss_media_data_err_inj_info_t expected_err_inj_data = {};
    expected_err_inj_data.err_rs_sym = 2; // Bit_Value; // some_expected_value
    expected_err_inj_data.err_inj_rw = 1;
    expected_err_inj_data.err_inj_beat = 1;
    expected_err_inj_data.err_inj_cnt = 1;
    expect_value(__wrap_ddrss_inject_media_data_err, mc, expected_mc);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_rs_sym, expected_err_inj_data.err_rs_sym);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_rw, expected_err_inj_data.err_inj_rw);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_beat, expected_err_inj_data.err_inj_beat);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_cnt, expected_err_inj_data.err_inj_cnt);
    will_return(__wrap_atu_map, 0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, 0);

    // Call the function under test
    ddr_ecc_error_injection(expected_die_num, expected_mc, p_addr, Bit_Value);
}

TEST_FUNCTION(test_ecc_ue_error_injection, NULL, NULL)
{
    // Set up pre-conditions / expectations
    const uint32_t p_addr = 0x0;
    const uint16_t Bit_Value = BIT1;
    const uint32_t expected_mc = 0;
    const int32_t expected_die_num = 0;
    ddrss_media_data_err_inj_info_t expected_err_inj_data = {};
    expected_err_inj_data.err_rs_sym = 2; // Bit_Value; // some_expected_value
    expected_err_inj_data.err_inj_rw = 1;
    expected_err_inj_data.err_inj_beat = 1;
    expected_err_inj_data.err_inj_cnt = 1;
    expect_value(__wrap_ddrss_inject_media_data_err, mc, expected_mc);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_rs_sym, expected_err_inj_data.err_rs_sym);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_rw, expected_err_inj_data.err_inj_rw);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_beat, expected_err_inj_data.err_inj_beat);
    expect_value(__wrap_ddrss_inject_media_data_err, media_err_inj->err_inj_cnt, expected_err_inj_data.err_inj_cnt);
    will_return(__wrap_atu_map, 0);
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    will_return(__wrap_atu_unmap, 0);

    // Call the function under test
    ddr_ecc_error_injection(expected_die_num, expected_mc, p_addr, Bit_Value);
}

TEST_FUNCTION(test_ddr_ca_parity_error_injection_happy_path, NULL, NULL)
{
    // Test valid commmand
    const uint32_t mc = 0;
    const DDRSS_MEDIA_CA_INJ_CMD valid_cmd = DDRSS_MEDIA_CA_INJ_CMD_WR;
    assert_int_equal(ddr_ca_parity_error_injection(mc, valid_cmd), SILIBS_SUCCESS);
}

TEST_FUNCTION(test_ddr_err_inj_media_patrol_scrub_ce_happy_path, NULL, NULL)
{
    g_should_check_ras_agent_entity_id = true;

    const uint32_t mc = 0;
    ras_agent_entity_t local_ras_agent = {0};
    local_ras_agent.flags = RAS_AGENT_FLAG_LIVE;

    expect_value(__wrap_ddrss_get_ras_agent, ras_agent_entity_id, ERG0);
    will_return(__wrap_ddrss_get_ras_agent, &local_ras_agent);
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    ddr_err_inj_media_patrol_scrub_ce(mc);

    g_should_check_ras_agent_entity_id = false;
}

TEST_FUNCTION(test_ddr_err_inj_fedb_merge_data_ce_happy_path, NULL, NULL)
{
    g_should_check_ras_agent_entity_id = true;

    const uint32_t mc = 0;
    ras_agent_entity_t local_ras_agent = {0};
    local_ras_agent.flags = RAS_AGENT_FLAG_LIVE;

    expect_value(__wrap_ddrss_get_ras_agent, ras_agent_entity_id, ERG0);
    will_return(__wrap_ddrss_get_ras_agent, &local_ras_agent);
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    ddr_err_inj_fedb_merge_data_ce(mc);

    g_should_check_ras_agent_entity_id = false;
}

TEST_FUNCTION(test_ddr_err_inj_media_patrol_scrub_ue_happy_path, NULL, NULL)
{
    g_should_check_ras_agent_entity_id = true;

    const uint32_t mc = 0;
    ras_agent_entity_t local_ras_agent = {0};
    local_ras_agent.flags = RAS_AGENT_FLAG_LIVE;

    expect_value(__wrap_ddrss_get_ras_agent, ras_agent_entity_id, ERG0);
    will_return(__wrap_ddrss_get_ras_agent, &local_ras_agent);
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    ddr_err_inj_media_patrol_scrub_ue(mc);

    g_should_check_ras_agent_entity_id = false;
}

TEST_FUNCTION(test_ddr_err_inj_mainline_traffic_ce_happy_path, NULL, NULL)
{
    g_should_check_ras_agent_entity_id = true;

    const uint32_t mc = 0;
    ras_agent_entity_t local_ras_agent = {0};
    local_ras_agent.flags = RAS_AGENT_FLAG_LIVE;

    expect_value(__wrap_ddrss_get_ras_agent, ras_agent_entity_id, ERG0);
    will_return(__wrap_ddrss_get_ras_agent, &local_ras_agent);
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    ddr_err_inj_mainline_traffic_ce(mc);

    g_should_check_ras_agent_entity_id = false;
}

TEST_FUNCTION(test_ddr_err_inj_mrdp_parity_ue_happy_path, NULL, NULL)
{
    g_should_check_ras_agent_entity_id = true;

    const uint32_t mc = 0;
    ras_agent_entity_t local_ras_agent = {0};
    local_ras_agent.flags = RAS_AGENT_FLAG_LIVE;

    expect_value(__wrap_ddrss_get_ras_agent, ras_agent_entity_id, ERG1);
    will_return(__wrap_ddrss_get_ras_agent, &local_ras_agent);
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    ddr_err_inj_mrdp_parity_ue(mc);

    g_should_check_ras_agent_entity_id = false;
}

TEST_FUNCTION(test_ddr_err_inj_mainline_traffic_ue_happy_path, NULL, NULL)
{
    g_should_check_ras_agent_entity_id = true;

    const uint32_t mc = 0;
    ras_agent_entity_t local_ras_agent = {0};
    local_ras_agent.flags = RAS_AGENT_FLAG_LIVE;

    expect_value(__wrap_ddrss_get_ras_agent, ras_agent_entity_id, ERG0);
    will_return(__wrap_ddrss_get_ras_agent, &local_ras_agent);
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    ddr_err_inj_mainline_traffic_ue(mc);

    g_should_check_ras_agent_entity_id = false;
}

TEST_FUNCTION(test_ddr_err_inj_ca_parity_persistent_happy_path, NULL, NULL)
{
    g_should_wrap_ddrss_inject_media_ca_err = true;

    const uint32_t mc = 0;
    expect_function_call(__wrap_ddrss_inject_media_ca_err);
    ddr_err_inj_ca_parity_persistent(mc);

    g_should_wrap_ddrss_inject_media_ca_err = false;
}

TEST_FUNCTION(test_ddr_err_inj_ca_parity_transient_happy_path, NULL, NULL)
{
    g_should_wrap_ddrss_get_config = true;
    g_should_wrap_ddrss_inject_media_ca_err = true;

    ddrss_cfg_knobs_t test_cfg_knobs = {};
    test_cfg_knobs.ext_knobs.ca_parity_err_recovery_en = 0; // Set to 0 to allow transient error injection
    will_return(__wrap_ddrss_get_config, &test_cfg_knobs);

    const uint32_t mc = 0;
    expect_function_call(__wrap_ddrss_inject_media_ca_err);
    ddr_err_inj_ca_parity_transient(mc);

    g_should_wrap_ddrss_inject_media_ca_err = false;
    g_should_wrap_ddrss_get_config = false;
}

TEST_FUNCTION(test_ddr_err_rh_counters_sram_parity_happy_path, NULL, NULL)
{
    g_should_check_ras_agent_entity_id = true;

    const uint32_t mc = 0;
    ras_agent_entity_t local_ras_agent = {0};
    local_ras_agent.flags = RAS_AGENT_FLAG_LIVE;

    expect_value(__wrap_ddrss_get_ras_agent, ras_agent_entity_id, ERG0);
    will_return(__wrap_ddrss_get_ras_agent, &local_ras_agent);
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    ddr_err_rh_counters_sram_parity(mc);

    g_should_check_ras_agent_entity_id = false;
}

TEST_FUNCTION(test_ddr_err_rh_drfm_sram_parity_happy_path, NULL, NULL)
{
    g_should_check_ras_agent_entity_id = true;

    const uint32_t mc = 0;
    ras_agent_entity_t local_ras_agent = {0};
    local_ras_agent.flags = RAS_AGENT_FLAG_LIVE;

    expect_value(__wrap_ddrss_get_ras_agent, ras_agent_entity_id, ERG0);
    will_return(__wrap_ddrss_get_ras_agent, &local_ras_agent);
    will_return(__wrap_ddrss_get_ras_agent, SILIBS_SUCCESS);
    ddr_err_rh_drfm_sram_parity(mc);

    g_should_check_ras_agent_entity_id = false;
}

TEST_FUNCTION(test_ddr_ca_parity_error_injection_bad_cmd, NULL, NULL)
{
    // Test invalid commmand
    const uint32_t mc = 0;
    const DDRSS_MEDIA_CA_INJ_CMD invalid_cmd = (DDRSS_MEDIA_CA_INJ_CMD)0x1FF;
    assert_int_equal(ddr_ca_parity_error_injection(mc, invalid_cmd), SILIBS_E_PARAM);
}

TEST_FUNCTION(test_ras_inj_syndrome_list, NULL, NULL)
{
    // const ras_inj_syndrome_t ras_syndrome_test[] = {DDR_ERR_INJ_SYNDROME_LIST(SYNDROME_STRUCT_INIT)};
    const ddr_err_inj_syndrome_t named_syndrome[] = {DDR_ERR_INJ_SYNDROME_LIST(NAMED_SYNDROME_STRUCT_INIT)};

    for (int i = 0; i < 2; i++)
    {
        printf("named_syndrome[%d].name: %s\n", i, named_syndrome[i].name);
        printf("named_syndrome[%d].erg: %d\n", i, named_syndrome[i].erg);
        printf("named_syndrome[%d].ras_arm_err_bitmask: %x\n", i, named_syndrome[i].ras_arm_err_bitmask);
        printf("named_syndrome[%d].addr: %llx\n", i, named_syndrome[i].syndrome.addr);
        printf("named_syndrome[%d].ierr: %d\n", i, named_syndrome[i].syndrome.ierr);
        printf("named_syndrome[%d].serr: %d\n", i, named_syndrome[i].syndrome.serr);
    }

    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_MREB_MAINLINE_TRAFFIC_CE"), 0);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_MREB_MAINLINE_TRAFFIC_UE"), 1);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_MREB_PATROL_SCRUB_CE"), 2);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_MREB_PATROL_SCRUB_UE"), 3);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_DATA_ARRAY_UE"), 4);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_DATA_UE"), 5);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_DATA_CE"), 6);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_DATA_PARITY_UE"), 7);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_STROBE_UE"), 8);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_MERGE_STROBE_PARITY_UE"), 9);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_FECQ_FEDB_STROBE_ARRAY_UE"), 10);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_HKE_PERSISTENT_CA_PARITY_UE"), 11);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_HKE_TRANSIENT_CA_PARITY_UE"), 12);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_RH_COUNTERS_SRAM_PARITY"), 13);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_RH_DRFM_SRAM_PARITY"), 14);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_XTS_AES_KEYSTORE_CE"), 15);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_XTS_AES_KEYSTORE_UE"), 16);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_BCP_READ_ADDR_NOT_IN_DDR"), 17);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_BCP_READ_BLOCKED_BY_PAS"), 18);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_BCP_WRITE_ADDR_NOT_IN_DDR"), 19);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_BCP_WRITE_BLOCKED_BY_PAS"), 20);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_BCP_CHI_UNSUPPORTED_OPCODE"), 21);
    assert_int_equal(get_syndrome_index("DDR_ERR_INJ_MRDP_PARITY_ERROR"), 22);
}
}