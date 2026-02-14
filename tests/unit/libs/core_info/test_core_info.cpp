//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_core_info.cpp
 * Core_info tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <core_info.h>
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fuse_init.h>
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#define __NO_CSR_TYPEDEFS__
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
#include <shared_crashdump_def.h>
#include <shared_sds_def.h> //Fuse SDS block and struct id
#include <silibs_platform.h>
#include <system_info.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

/*------------- Functions ----------------*/
silibs_status_t __wrap_read_core_defect_fuses(uint32_t* fuse_dis_core_64_67, uint32_t* fuse_dis_core_63_32, uint32_t* fuse_dis_core_31_0)
{
    if (fuse_dis_core_64_67)
        *fuse_dis_core_64_67 = 0;
    if (fuse_dis_core_63_32)
        *fuse_dis_core_63_32 = 0;
    if (fuse_dis_core_31_0)
        *fuse_dis_core_31_0 = 0;
    return mock_type(silibs_status_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

KNG_CPU_TYPE __wrap_idsw_get_cpu_type(void)
{
    return mock_type(KNG_CPU_TYPE);
}

uint32_t __wrap_config_get_die0_core_disable_value_31_0()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die0_core_disable_value_63_32()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die0_core_disable_value_95_64()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die0_core_spare_en_31_0()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die0_core_spare_en_63_32()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die0_core_spare_en_95_64()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die1_core_disable_value_31_0()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die1_core_disable_value_63_32()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die1_core_disable_value_95_64()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die1_core_spare_en_31_0()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die1_core_spare_en_63_32()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_die1_core_spare_en_95_64()
{
    return mock_type(uint32_t);
}

TEST_FUNCTION(test_core_info_get_disable_cores_result_die0, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    // Fuse all 0；Config seperately setup 0x0, 0x1, 0x3
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_die0_core_disable_value_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_63_32, 0x00000001);
    will_return(__wrap_config_get_die0_core_disable_value_95_64, 0x00000003);

    will_return(__wrap_config_get_die0_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_63_32, 0x000000000);
    will_return(__wrap_config_get_die0_core_spare_en_95_64, 0x00000000);

    // calculation
    core_info_get_platform_disable_cores();

    // get the result
    corebits_t* result = core_info_get_enable_cores_result();

    assert_true(corebits_is_bit_set(result, 0));
    assert_true(corebits_is_bit_set(result, 31));
    assert_false(corebits_is_bit_set(result, 32));
    assert_true(corebits_is_bit_set(result, 33));
    assert_true(corebits_is_bit_set(result, 66));
    assert_false(corebits_is_bit_set(result, 65));
}

TEST_FUNCTION(test_core_info_get_spare_en_cores_die0, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    // Fuse all 0；Config separately setup 0x2, 0x1, 0x3
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_die0_core_disable_value_31_0, 0x00000002);
    will_return(__wrap_config_get_die0_core_disable_value_63_32, 0x00000001);
    will_return(__wrap_config_get_die0_core_disable_value_95_64, 0x0000000F);

    // Enable Core 1, Core 66, Core 67
    will_return(__wrap_config_get_die0_core_spare_en_31_0, 0x00000002);
    will_return(__wrap_config_get_die0_core_spare_en_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_95_64, 0x0000000C);

    // calculation
    core_info_get_platform_disable_cores();
    // get the result
    corebits_t* result = core_info_get_enable_cores_result();

    assert_true(corebits_is_bit_set(result, 0));
    assert_true(corebits_is_bit_set(result, 2));
    assert_true(corebits_is_bit_set(result, 31));
    assert_false(corebits_is_bit_set(result, 32));
    assert_true(corebits_is_bit_set(result, 33));
    assert_false(corebits_is_bit_set(result, 64));
    assert_false(corebits_is_bit_set(result, 65));
}

TEST_FUNCTION(test_core_info_get_disable_cores_result_die1, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    // Fuse all 0；Config seperately setup 0x0, 0x1, 0x3
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_die1_core_disable_value_31_0, 0x00000000);
    will_return(__wrap_config_get_die1_core_disable_value_63_32, 0x00000001);
    will_return(__wrap_config_get_die1_core_disable_value_95_64, 0x00000003);

    will_return(__wrap_config_get_die1_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die1_core_spare_en_63_32, 0x000000000);
    will_return(__wrap_config_get_die1_core_spare_en_95_64, 0x00000000);

    // calculation
    core_info_get_platform_disable_cores();

    // get the result
    corebits_t* result = core_info_get_enable_cores_result();

    assert_true(corebits_is_bit_set(result, 0));
    assert_true(corebits_is_bit_set(result, 31));
    assert_false(corebits_is_bit_set(result, 32));
    assert_true(corebits_is_bit_set(result, 33));
    assert_true(corebits_is_bit_set(result, 66));
    assert_false(corebits_is_bit_set(result, 65));
}

TEST_FUNCTION(test_core_info_get_spare_en_cores_die1, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    // Fuse all 0；Config separately setup 0x2, 0x1, 0x3
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_die1_core_disable_value_31_0, 0x00000002);
    will_return(__wrap_config_get_die1_core_disable_value_63_32, 0x00000001);
    will_return(__wrap_config_get_die1_core_disable_value_95_64, 0x0000000F);

    // Enable Core 1, Core 66, Core 67
    will_return(__wrap_config_get_die1_core_spare_en_31_0, 0x00000002);
    will_return(__wrap_config_get_die1_core_spare_en_63_32, 0x00000000);
    will_return(__wrap_config_get_die1_core_spare_en_95_64, 0x0000000C);

    // calculation
    core_info_get_platform_disable_cores();
    // get the result
    corebits_t* result = core_info_get_enable_cores_result();

    assert_true(corebits_is_bit_set(result, 0));
    assert_true(corebits_is_bit_set(result, 2));
    assert_true(corebits_is_bit_set(result, 31));
    assert_false(corebits_is_bit_set(result, 32));
    assert_true(corebits_is_bit_set(result, 33));
    assert_false(corebits_is_bit_set(result, 64));
    assert_false(corebits_is_bit_set(result, 65));
}
}

TEST_FUNCTION(test_core_info_fuse_disable_core_to_66, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    // Fuse all 0; Config separately setup 0x0, 0x0, 0x0
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_die0_core_disable_value_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_95_64, 0x00000000);

    will_return(__wrap_config_get_die0_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_95_64, 0x00000000);

    // calculation
    core_info_get_platform_disable_cores();

    // get the result
    corebits_t* result = core_info_get_enable_cores_result();

    // Core 26 and Core 37 should be disabled, so they must NOT be set in result
    assert_false(corebits_is_bit_set(result, 26));
    assert_false(corebits_is_bit_set(result, 37));
}

TEST_FUNCTION(test_core_info_fuse_disable_1_core, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_die0_core_disable_value_31_0, 0x00000010); // Core 4 disabled
    will_return(__wrap_config_get_die0_core_disable_value_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_95_64, 0x00000000);

    will_return(__wrap_config_get_die0_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_95_64, 0x00000000);

    // calculation
    core_info_get_platform_disable_cores();

    // get the result
    corebits_t* result = core_info_get_enable_cores_result();

    // Core 37 should be disabled by the pickup algorithm, so it must NOT be set in result
    assert_false(corebits_is_bit_set(result, 37));
}

/*
 * Tests for etc_core_identifier_t union and core_info_get_combined_core_id
 */

TEST_FUNCTION(test_etc_core_identifier_union_layout, nullptr, nullptr)
{
    // Test that the union correctly maps core_id to bits 15:0 and die_id to bits 31:16
    etc_core_identifier_t id = {};

    // Set die_id = 0x1234, core_id = 0x5678
    id.die_id = 0x1234;
    id.core_id = 0x5678;

    // Combined should be 0x12345678 (die_id in MSB, core_id in LSB)
    assert_int_equal(id.combined_id, 0x12345678);
}

TEST_FUNCTION(test_etc_core_identifier_union_extract_fields, nullptr, nullptr)
{
    // Test extracting fields from combined_id
    etc_core_identifier_t id = {};

    id.combined_id = 0xABCD1234;

    // die_id should be in bits 31:16 = 0xABCD
    assert_int_equal(id.die_id, 0xABCD);
    // core_id should be in bits 15:0 = 0x1234
    assert_int_equal(id.core_id, 0x1234);
}

TEST_FUNCTION(test_etc_core_identifier_union_boundary_values, nullptr, nullptr)
{
    etc_core_identifier_t id = {};

    // Test max values
    id.die_id = 0xFFFF;
    id.core_id = 0xFFFF;
    assert_int_equal(id.combined_id, 0xFFFFFFFF);

    // Test min values
    id.die_id = 0x0000;
    id.core_id = 0x0000;
    assert_int_equal(id.combined_id, 0x00000000);

    // Test die_id only
    id.die_id = 0x0001;
    id.core_id = 0x0000;
    assert_int_equal(id.combined_id, 0x00010000);

    // Test core_id only
    id.die_id = 0x0000;
    id.core_id = 0x0001;
    assert_int_equal(id.combined_id, 0x00000001);
}

TEST_FUNCTION(test_core_info_init_mcp_die0, nullptr, nullptr)
{
    // Setup mocks for core_info_get_platform_disable_cores
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return(__wrap_idsw_get_cpu_type, CPU_MCP);

    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_die0_core_disable_value_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_95_64, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_95_64, 0x00000000);

    core_info_init();

    uint32_t combined_id = core_info_get_combined_core_id();

    // Extract die_id (bits 31:16) and core_id (bits 15:0)
    uint16_t die_id = (uint16_t)(combined_id >> 16);
    uint16_t core_id = (uint16_t)(combined_id & 0xFFFF);

    assert_int_equal(die_id, DIE_0);
    assert_int_equal(core_id, CRASH_DUMP_CORE_MCP);
}

TEST_FUNCTION(test_core_info_init_scp_die1, nullptr, nullptr)
{
    // Setup mocks for core_info_get_platform_disable_cores
    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_idsw_get_cpu_type, CPU_SCP);

    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_die1_core_disable_value_31_0, 0x00000000);
    will_return(__wrap_config_get_die1_core_disable_value_63_32, 0x00000000);
    will_return(__wrap_config_get_die1_core_disable_value_95_64, 0x00000000);
    will_return(__wrap_config_get_die1_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die1_core_spare_en_63_32, 0x00000000);
    will_return(__wrap_config_get_die1_core_spare_en_95_64, 0x00000000);

    core_info_init();

    uint32_t combined_id = core_info_get_combined_core_id();

    // Extract die_id (bits 31:16) and core_id (bits 15:0)
    uint16_t die_id = (uint16_t)(combined_id >> 16);
    uint16_t core_id = (uint16_t)(combined_id & 0xFFFF);

    assert_int_equal(die_id, DIE_1);
    assert_int_equal(core_id, CRASH_DUMP_CORE_SCP);
}
