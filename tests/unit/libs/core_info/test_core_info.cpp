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
silibs_status_t __wrap_read_core_defect_fuses(uint32_t* fuse_dis_core_64_67, uint32_t* fuse_dis_core_32_63, uint32_t* fuse_dis_core_0_31)
{
    if (fuse_dis_core_64_67)
    {
        *fuse_dis_core_64_67 = 0; // Simulate fuse value of 0.
    }
    if (fuse_dis_core_32_63)
    {
        *fuse_dis_core_32_63 = 0; // Simulate fuse value of 0.
    }
    if (fuse_dis_core_0_31)
    {
        *fuse_dis_core_0_31 = 0; // Simulate fuse value of 0.
    }
    return mock_type(silibs_status_t);
}

uint32_t __wrap_config_get_core_disable_value_0_31()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_core_disable_value_32_63()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_core_disable_value_64_95()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_core_spare_en_0_31()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_core_spare_en_32_63()
{
    return mock_type(uint32_t);
}

uint32_t __wrap_config_get_core_spare_en_64_95()
{
    return mock_type(uint32_t);
}

TEST_FUNCTION(test_core_info_get_disable_cores_result, nullptr, nullptr)
{
    // Fuse all 0；Config seperately setup 0x0, 0x1, 0x3
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_core_disable_value_0_31, 0x00000000);
    will_return(__wrap_config_get_core_disable_value_32_63, 0x00000001);
    will_return(__wrap_config_get_core_disable_value_64_95, 0x00000003);

    will_return(__wrap_config_get_core_spare_en_0_31, 0x00000000);
    will_return(__wrap_config_get_core_spare_en_32_63, 0x000000000);
    will_return(__wrap_config_get_core_spare_en_64_95, 0x00000000);

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

TEST_FUNCTION(test_core_info_get_spare_en_cores, nullptr, nullptr)
{
    // Fuse all 0；Config seperately setup 0x2, 0x1, 0x3
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    will_return(__wrap_config_get_core_disable_value_0_31, 0x00000002);
    will_return(__wrap_config_get_core_disable_value_32_63, 0x00000001);
    will_return(__wrap_config_get_core_disable_value_64_95, 0x0000000F);

    // Enable Core 1, Core 66, Core 67
    will_return(__wrap_config_get_core_spare_en_0_31, 0x00000002);
    will_return(__wrap_config_get_core_spare_en_32_63, 0x00000000);
    will_return(__wrap_config_get_core_spare_en_64_95, 0x0000000C);

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