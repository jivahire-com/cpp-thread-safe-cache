//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cxl.cpp
 * cxl library tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h>

extern "C" {
#include <FpFwUtils.h>
#include <cxl.h>
#include <fpfw_cfg_mgr.h>
#include <string.h>

cxl_region_params_t __wrap_config_get_cxl_params_die0(void)
{
    cxl_region_params_t* params = mock_ptr_type(cxl_region_params_t*);
    return *params;
}

cxl_region_params_t __wrap_config_get_cxl_params_die1(void)
{
    cxl_region_params_t* params = mock_ptr_type(cxl_region_params_t*);
    return *params;
}
}

static int setup(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

static int teardown(void** state)
{
    FPFW_UNUSED(state);
    return 0;
}

// Test set_cxl_mem_region_base_and_size with a single target
TEST_FUNCTION(test_set_cxl_mem_region_base_and_size_single, setup, teardown)
{
    cxl_hdm_decoder_region_t region = {{0}};
    uint64_t base_addr = 0x10101000000000;
    uint8_t num_targets = 1;

    uint64_t ret_size = set_cxl_mem_region_base_and_size(&region, &base_addr, num_targets);

    uint64_t result_base = (region.base_low.as_uint32 & 0xF) | ((uint64_t)region.base_high.base_high << 32);
    assert_int_equal(result_base, base_addr);

    uint64_t result_size = (region.size_low.as_uint32 & 0xF) | ((uint64_t)region.size_high.size_high << 32);
    assert_int_equal(result_size, CXL_MEM_REGION_SIZE);
    assert_int_equal(ret_size, CXL_MEM_REGION_SIZE);
}

// Test set_cxl_mem_region_base_and_size with multiple interleaved targets
TEST_FUNCTION(test_set_cxl_mem_region_base_and_size_interleaved, setup, teardown)
{
    cxl_hdm_decoder_region_t region = {{0}};
    uint64_t base_addr = 0x10101000000000;
    uint8_t num_targets = 2; // example of interleaving

    uint64_t ret_size = set_cxl_mem_region_base_and_size(&region, &base_addr, num_targets);

    uint64_t result_base = (region.base_low.as_uint32 & 0xF) | ((uint64_t)region.base_high.base_high << 32);
    assert_int_equal(result_base, base_addr);

    uint64_t result_size = (region.size_low.as_uint32 & 0xF) | ((uint64_t)region.size_high.size_high << 32);
    assert_int_equal(result_size, 0x8000000000); // 512 for 2 targets
    assert_int_equal(ret_size, 0x8000000000);
}

// Test cxl_chbcr_init 2 separate regions - no interleaving
TEST_FUNCTION(test_cxl_chbcr_init_2_regions_no_interleave, setup, teardown)
{
    // create mock params
    cxl_region_params_t params_die_0 = {0};
    params_die_0.valid = false;
    cxl_region_params_t params_die_1 = {0};
    params_die_1.valid = true;
    params_die_1.interleave_ways = INTERLEAVE_NONE;
    params_die_1.ports[0] = CXL_RPSS4_RP0;
    params_die_1.ports[1] = CXL_RPSS5_RP0;
    params_die_1.ports[2] = CXL_PORT_EMPTY;
    params_die_1.ports[3] = CXL_PORT_EMPTY;

    will_return(__wrap_config_get_cxl_params_die0, &params_die_0);
    will_return(__wrap_config_get_cxl_params_die1, &params_die_1);

    // Allocate some buffer to simulate CHBCR region
    uint8_t chbcr_area[512] = {0};
    cxl_chbcr_init(chbcr_area);
    // We can check some fields to ensure they were set
    cxl_chbcr_header_t* hdr = (cxl_chbcr_header_t*)chbcr_area;
    assert_int_equal(hdr->capability_reg.target_count, 4);
    assert_int_equal(hdr->capability_reg.a11to8_interleave_capable, 1);
    assert_int_equal(hdr->capability_reg.a14to12_interleave_capable, 1);

    cxl_hdm_decoder_region_t* region = (cxl_hdm_decoder_region_t*)(chbcr_area + sizeof(cxl_chbcr_header_t));
    uint64_t result_base = (region->base_low.as_uint32 & 0xF) | ((uint64_t)region->base_high.base_high << 32);
    assert_int_equal(result_base, CXL_MEM_REGION_BASE_ADDR);
    uint64_t result_size = (region->size_low.as_uint32 & 0xF) | ((uint64_t)region->size_high.size_high << 32);
    assert_int_equal(result_size, CXL_MEM_REGION_SIZE);
    assert_int_equal(region->target_list.target_port_id[0], config_get_pcie_rpss4_rp0_cfg().pcie_rp_port_num);

    region++;
    result_base = (region->base_low.as_uint32 & 0xF) | ((uint64_t)region->base_high.base_high << 32);
    assert_int_equal(result_base, CXL_MEM_REGION_BASE_ADDR + CXL_MEM_REGION_SIZE);
    result_size = (region->size_low.as_uint32 & 0xF) | ((uint64_t)region->size_high.size_high << 32);
    assert_int_equal(result_size, CXL_MEM_REGION_SIZE);
    assert_int_equal(region->target_list.target_port_id[0], config_get_pcie_rpss5_rp0_cfg().pcie_rp_port_num);
}

// Test cxl_chbcr_init one interleaved region
TEST_FUNCTION(test_cxl_chbcr_init_one_region_interleave, setup, teardown)
{
    // create mock params
    cxl_region_params_t params_die_0 = {0};
    params_die_0.valid = false;
    cxl_region_params_t params_die_1 = {0};
    params_die_1.valid = true;
    params_die_1.interleave_ways = INTERLEAVE_4_WAYS;
    params_die_1.ports[0] = CXL_RPSS4_RP0;
    params_die_1.ports[1] = CXL_RPSS5_RP0;
    params_die_1.ports[2] = CXL_RPSS6_RP0;
    params_die_1.ports[3] = CXL_RPSS7_RP0;

    will_return(__wrap_config_get_cxl_params_die0, &params_die_0);
    will_return(__wrap_config_get_cxl_params_die1, &params_die_1);

    // Allocate some buffer to simulate CHBCR region
    uint8_t chbcr_area[512] = {0};
    cxl_chbcr_init(chbcr_area);
    // We can check some fields to ensure they were set
    cxl_chbcr_header_t* hdr = (cxl_chbcr_header_t*)chbcr_area;
    assert_int_equal(hdr->capability_reg.target_count, 4);
    assert_int_equal(hdr->capability_reg.a11to8_interleave_capable, 1);
    assert_int_equal(hdr->capability_reg.a14to12_interleave_capable, 1);

    cxl_hdm_decoder_region_t* region = (cxl_hdm_decoder_region_t*)(chbcr_area + sizeof(cxl_chbcr_header_t));
    uint64_t result_base = (region->base_low.as_uint32 & 0xF) | ((uint64_t)region->base_high.base_high << 32);
    assert_int_equal(result_base, CXL_MEM_REGION_BASE_ADDR);
    uint64_t result_size = (region->size_low.as_uint32 & 0xF) | ((uint64_t)region->size_high.size_high << 32);
    assert_int_equal(result_size, CXL_MEM_REGION_SIZE * 4); // 4 targets
    assert_int_equal(region->target_list.target_port_id[0], config_get_pcie_rpss4_rp0_cfg().pcie_rp_port_num);
    assert_int_equal(region->target_list.target_port_id[1], config_get_pcie_rpss5_rp0_cfg().pcie_rp_port_num);
    assert_int_equal(region->target_list.target_port_id[2], config_get_pcie_rpss6_rp0_cfg().pcie_rp_port_num);
    assert_int_equal(region->target_list.target_port_id[3], config_get_pcie_rpss7_rp0_cfg().pcie_rp_port_num);
}

// Test both interleaved and non-interleaved regions
TEST_FUNCTION(test_cxl_chbcr_init_both, setup, teardown)
{
    // create mock params
    cxl_region_params_t params_die_0 = {0};
    params_die_0.valid = true;
    params_die_0.interleave_ways = INTERLEAVE_NONE;
    params_die_0.ports[0] = CXL_RPSS0_RP0;
    params_die_0.ports[1] = CXL_RPSS1_RP0;
    params_die_0.ports[2] = CXL_PORT_EMPTY;
    params_die_0.ports[3] = CXL_PORT_EMPTY;

    cxl_region_params_t params_die_1 = {0};
    params_die_1.valid = true;
    params_die_1.interleave_ways = INTERLEAVE_2_WAYS;
    params_die_1.ports[0] = CXL_RPSS6_RP0;
    params_die_1.ports[1] = CXL_RPSS7_RP0;
    params_die_1.ports[2] = CXL_PORT_EMPTY;
    params_die_1.ports[3] = CXL_PORT_EMPTY;

    will_return(__wrap_config_get_cxl_params_die0, &params_die_0);
    will_return(__wrap_config_get_cxl_params_die1, &params_die_1);

    // Allocate some buffer to simulate CHBCR region
    uint8_t chbcr_area[512] = {0};
    cxl_chbcr_init(chbcr_area);
    // We can check some fields to ensure they were set
    cxl_chbcr_header_t* hdr = (cxl_chbcr_header_t*)chbcr_area;
    assert_int_equal(hdr->capability_reg.target_count, 4);
    assert_int_equal(hdr->capability_reg.a11to8_interleave_capable, 1);
    assert_int_equal(hdr->capability_reg.a14to12_interleave_capable, 1);

    cxl_hdm_decoder_region_t* region = (cxl_hdm_decoder_region_t*)(chbcr_area + sizeof(cxl_chbcr_header_t));

    uint64_t result_base = (region->base_low.as_uint32 & 0xF) | ((uint64_t)region->base_high.base_high << 32);
    assert_int_equal(result_base, CXL_MEM_REGION_BASE_ADDR);
    uint64_t result_size = (region->size_low.as_uint32 & 0xF) | ((uint64_t)region->size_high.size_high << 32);
    assert_int_equal(result_size, CXL_MEM_REGION_SIZE);
    assert_int_equal(region->target_list.target_port_id[0], config_get_pcie_rpss0_rp0_cfg().pcie_rp_port_num);
    region++;

    result_base = (region->base_low.as_uint32 & 0xF) | ((uint64_t)region->base_high.base_high << 32);
    assert_int_equal(result_base, CXL_MEM_REGION_BASE_ADDR + CXL_MEM_REGION_SIZE);
    result_size = (region->size_low.as_uint32 & 0xF) | ((uint64_t)region->size_high.size_high << 32);
    assert_int_equal(result_size, CXL_MEM_REGION_SIZE);
    assert_int_equal(region->target_list.target_port_id[0], config_get_pcie_rpss1_rp0_cfg().pcie_rp_port_num);
    region++;

    result_base = (region->base_low.as_uint32 & 0xF) | ((uint64_t)region->base_high.base_high << 32);
    assert_int_equal(result_base, CXL_MEM_REGION_BASE_ADDR + 2 * CXL_MEM_REGION_SIZE);
    result_size = (region->size_low.as_uint32 & 0xF) | ((uint64_t)region->size_high.size_high << 32);
    assert_int_equal(result_size, CXL_MEM_REGION_SIZE * 2);
    assert_int_equal(region->target_list.target_port_id[0], config_get_pcie_rpss6_rp0_cfg().pcie_rp_port_num);
    assert_int_equal(region->target_list.target_port_id[1], config_get_pcie_rpss7_rp0_cfg().pcie_rp_port_num);
}
