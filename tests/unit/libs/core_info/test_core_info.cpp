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
#include <fpfw_icc_base.h>        // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <hsp_firmware_headers.h> // for HSP_FIRMWARE_ID
#define __NO_CSR_TYPEDEFS__
#include <scp_exp_top_regs.h>
#include <scp_top_regs.h>
#undef __NO_CSR_TYPEDEFS__
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
    {
        *fuse_dis_core_64_67 = 0; // Simulate fuse value of 0.
    }
    if (fuse_dis_core_63_32)
    {
        *fuse_dis_core_63_32 = 0; // Simulate fuse value of 0.
    }
    if (fuse_dis_core_31_0)
    {
        *fuse_dis_core_31_0 = 0; // Simulate fuse value of 0.
    }
    return mock_type(silibs_status_t);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
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
    // Fuse all 0；Config seperately setup 0x2, 0x1, 0x3
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
    assert_true(corebits_is_bit_set(result, 66));
    assert_true(corebits_is_bit_set(result, 67));
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
    // Fuse all 0；Config seperately setup 0x2, 0x1, 0x3
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
    assert_true(corebits_is_bit_set(result, 66));
    assert_true(corebits_is_bit_set(result, 67));
}
}

TEST_FUNCTION(test_core_info_fuse_disable_core_to_66, nullptr, nullptr)
{
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    // Fuse all 0；Config seperately setup 0x0, 0x1, 0x3
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_die0_core_disable_value_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_95_64, 0x00000000);

    will_return(__wrap_config_get_die0_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_63_32, 0x000000000);
    will_return(__wrap_config_get_die0_core_spare_en_95_64, 0x00000000);

    // calculation
    core_info_get_platform_disable_cores();

    // get the result
    corebits_t* result = core_info_get_enable_cores_result();

    assert_false(corebits_is_bit_set(result, 26));
    assert_false(corebits_is_bit_set(result, 37));
}
