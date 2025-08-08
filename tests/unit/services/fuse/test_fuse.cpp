/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 */

/**
 * @file test_fuse.cpp
 *
 * Provides the mocked version of
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstddef>         // for NULL
#include <cstdint>         // for uintptr_t

extern "C" {
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <fpfw_cfg_mgr.h>
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fpfw_init.h>     // for fpfw_init_get_handle, FPFW_INIT_C...
#include <fpfw_status.h>   // for fpfw_status_t
#include <fuse.h>
#include <fuse_client.h>
#include <fuse_dist_platform_exclusions.h>
#include <fuse_init.h>
#include <fuse_main_i.h>
#include <fuse_struct.h>
#include <fuses_top_regs.h>
#include <hsp_firmware_headers.h>
#include <icc_mhu.h>
#include <icc_platform_defines.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h> // Test revision get
#include <kng_icc_shared.h>
#include <memory_map/mscp_exp_rmss_memory_map.h>
#include <sds_api.h>
#include <sds_configuration.h>
#include <sds_init.h>
#include <setjmp.h>
#include <shared_sds_def.h> //Fuse SDS block and struct id
#include <silibs_platform.h>
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdnoreturn.h>
#include <system_info.h>
#include <utils.h> // for UNUSED

/*-- Symbolic Constant Macros (defines) --*/
#define BUGCHECK_MOCK_RETURN() (setjmp(cd_mock_jump_buf))
#define FUSE_PAYLOAD_SIZE      7

/*------------- Typedefs -----------------*/
typedef struct
{
    icc_mhu_header_t header;
    uint32_t payload[FUSE_PAYLOAD_SIZE];
} icc_mhu_d2d_fuse_packet_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

static jmp_buf cd_mock_jump_buf;
static kng_fuse_disable_core_t DIE0_fuse_disable_test = {0x00, 0x00, 0x0, 0xFFFFFFFF};
static kng_fuse_disable_core_t DIE1_core_disable_post_knob_test = {0x04000000, 0x40, 0xfffffff0, 0xffffffff};

/*------------- Functions ----------------*/

//
// Mocks

KNG_PLAT_ID __wrap_idsw_get_platform_sdv()
{
    return mock_type(KNG_PLAT_ID);
}

KNG_DIE_ID __wrap_idsw_get_die_id(void)
{
    return mock_type(KNG_DIE_ID);
}

bool __wrap_idhw_is_single_die_boot_en(void)
{
    return mock_type(bool);
}

int __wrap_mesh_get_hns_sds_vector_from_hns_sparring(kng_hns_fuses_t* hns_fuses_sds)
{
    UNUSED(hns_fuses_sds);
    return mock_type(int);
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

silibs_status_t __wrap_fuse_dma_copy_to_ram_blocking()
{
    // function_called();
    return mock_type(int);
}
int __wrap_fuse_read(const unsigned fuse_bit_offset, const unsigned fuse_bit_size)
{
    check_expected(fuse_bit_offset);
    check_expected(fuse_bit_size);
    // function_called();
    return mock_type(int);
}
silibs_status_t __wrap_read_core_defect_fuses(uint32_t* fuse_dis_core_67_64, uint32_t* fuse_dis_core_63_32, uint32_t* fuse_dis_core_31_0)
{
    check_expected_ptr(fuse_dis_core_67_64);
    check_expected_ptr(fuse_dis_core_63_32);
    check_expected_ptr(fuse_dis_core_31_0);
    return mock_type(silibs_status_t);
}
silibs_status_t __wrap_fuse_override(KNG_DIE_ID die_id, const uintptr_t override_buffer)
{
    check_expected(die_id);
    check_expected_ptr(override_buffer);
    function_called();
    return SILIBS_SUCCESS;
}

silibs_status_t __wrap_fuse_override_ignoring_valids(idsw_die_id_t die_id, const uintptr_t override_buffer)
{
    check_expected(die_id);
    check_expected_ptr(override_buffer);
    function_called();
    return 0;
}

silibs_status_t __wrap_fuse_distribution(KNG_DIE_ID die_id,
                                         const FUSE_DISTRIBUTION_MAJOR_PHASE phase_maj,
                                         const FUSE_DISTRIBUTION_MINOR_PHASE phase_min,
                                         const fuse_dist_exclude_range_t* exclude_list,
                                         const uint32_t exclude_list_count)
{
    check_expected(die_id);
    check_expected(phase_maj);
    check_expected(phase_min);
    check_expected_ptr(exclude_list);
    check_expected(exclude_list_count);

    // Debug prints
    printf("In __wrap_fuse_distribution:\n");
    for (uint32_t i = 0; i < exclude_list_count; ++i)
    {
        printf("Exclusion list [%u]: start_addr=%llu, end_addr=%llu\n",
               i,
               exclude_list[i].start_addr,
               exclude_list[i].end_addr);
    }
    function_called();
    return SILIBS_SUCCESS;
}

silibs_status_t __wrap_trigger_debugger_for_manual_overrides()
{
    function_called();
    return SILIBS_SUCCESS;
}

silibs_status_t __wrap_fuse_get_exclude_list_soc_nano(const fuse_dist_exclude_range_t** IP_exclude_list, uint32_t* IP_exclude_count)
{
    check_expected_ptr(IP_exclude_list);
    check_expected_ptr(IP_exclude_count);
    *IP_exclude_list = (const fuse_dist_exclude_range_t*)mock_ptr_type(const fuse_dist_exclude_range_t*);
    *IP_exclude_count = mock_type(uint32_t);
    function_called();
    return SILIBS_SUCCESS;
}

silibs_status_t __wrap_fuse_dist_get_exclusion_list_secure(KNG_DIE_ID die_id,
                                                           KNG_PLAT_ID plat_id,
                                                           const fuse_dist_exclude_range_t** IP_exclude_list,
                                                           uint32_t* IP_exclude_count,
                                                           bool is_secure)
{
    check_expected(die_id);
    check_expected(plat_id);
    check_expected_ptr(IP_exclude_list);
    check_expected_ptr(IP_exclude_count);
    check_expected(is_secure);
    *IP_exclude_list = (const fuse_dist_exclude_range_t*)mock_ptr_type(const fuse_dist_exclude_range_t*);
    *IP_exclude_count = mock_type(uint32_t);
    function_called();
    return SILIBS_SUCCESS;
}

noreturn void __wrap_crash_dump_bug_check(uint32_t errorCode, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4)
{
    check_expected(errorCode);
    FPFW_UNUSED(p1);
    FPFW_UNUSED(p2);
    FPFW_UNUSED(p3);
    FPFW_UNUSED(p4);

    // Handle noreturn, allowing control to return to test
    longjmp(cd_mock_jump_buf, 1);
}
fpfw_icc_base_ctx_t* __wrap_fpfw_init_get_handle(const char* name)
{
    check_expected_ptr(name);
    function_called();
    return mock_ptr_type(fpfw_icc_base_ctx_t*);
}
fpfw_status_t __wrap_fpfw_icc_base_send_recv_sync(fpfw_icc_base_ctx_t* icc_ctx, void* payload_buffer, size_t buffer_size, size_t* output_recv_bytes)
{
    FPFW_UNUSED(icc_ctx);
    FPFW_UNUSED(payload_buffer);
    check_expected(buffer_size);
    check_expected_ptr(output_recv_bytes);

    kng_hsp_fuse_mailbox_msg* recv_msg = (kng_hsp_fuse_mailbox_msg*)payload_buffer;
    recv_msg->fuse_req.header.cmd = HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_RSP;
    recv_msg->rsp.status = HSP_MAILBOX_RSP_STATUS_SUCCESS;
    *output_recv_bytes = sizeof(kng_hsp_fuse_mailbox_msg);

    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    FPFW_UNUSED(icc_ctx);

    assert_true(params != NULL);
    assert_true(params->payload_buffer != NULL);
    assert_true(params->buffer_size == 0x200);

    function_called();
    if (params->cb != NULL)
    {
        params->cb(params->cb_ctx, FPFW_ICC_BASE_STATUS_SUCCESS);
    }
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    FPFW_UNUSED(icc_ctx);
    check_expected(params->recv_cmd_code);

    return mock_type(fpfw_status_t);
}

int32_t __wrap_sds_block_write(uint32_t sds_module_id, void* buffer, size_t buffer_size)
{
    check_expected(sds_module_id);
    check_expected_ptr(buffer);
    check_expected(buffer_size);

    return 0;
}

int32_t __wrap_sds_block_creation(uint32_t sds_module_id, uint32_t request_size, uint32_t region_id)
{
    FPFW_UNUSED(sds_module_id);
    FPFW_UNUSED(request_size);
    FPFW_UNUSED(region_id);

    return 0;
}

bool __wrap_system_info_is_warm_start()
{
    return mock_type(bool);
}

hsp_security_state_t __wrap_system_info_get_security_state()
{
    return mock_type(hsp_security_state_t);
}
//
// Tests
//
// test fuse_read

TEST_FUNCTION(test_fuse_override_SIM, NULL, NULL)
{

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);

    will_return_always(__wrap_fuse_dma_copy_to_ram_blocking, 0);
    expect_value(__wrap_fuse_read, fuse_bit_offset, SILICON_ID_SILICON_MAJOR_REVISION_BIT_OFFSET);
    expect_value(__wrap_fuse_read, fuse_bit_size, SILICON_ID_SILICON_MAJOR_REVISION_WIDTH);
    will_return(__wrap_fuse_read, 1);

    // Expectation for fuse_override
    expect_value(__wrap_fuse_override, die_id, DIE_0);
    expect_value(__wrap_fuse_override, override_buffer, (uintptr_t)(SCP_EXP_FUSE_DATA_BASE));
    expect_function_call(__wrap_fuse_override);

    will_return(__wrap_system_info_is_warm_start, false);
    expect_memory(__wrap_read_core_defect_fuses,
                  fuse_dis_core_67_64,
                  &(DIE0_fuse_disable_test.fuse_dis_core_95_64),
                  sizeof(DIE0_fuse_disable_test.fuse_dis_core_95_64));
    expect_memory(__wrap_read_core_defect_fuses,
                  fuse_dis_core_63_32,
                  &(DIE0_fuse_disable_test.fuse_dis_core_63_32),
                  sizeof(DIE0_fuse_disable_test.fuse_dis_core_63_32));
    expect_memory(__wrap_read_core_defect_fuses,
                  fuse_dis_core_31_0,
                  &(DIE0_fuse_disable_test.fuse_dis_core_31_0),
                  sizeof(DIE0_fuse_disable_test.fuse_dis_core_31_0));
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);

    expect_function_call(__wrap_trigger_debugger_for_manual_overrides);

    assert_int_equal(platform_fuse_override(), 0);
}

TEST_FUNCTION(test_read_fuse, NULL, NULL)
{
    uint64_t fuse_data = 0;
    expect_value(__wrap_fuse_read, fuse_bit_offset, SILICON_ID_SILICON_MAJOR_REVISION_BIT_OFFSET);
    expect_value(__wrap_fuse_read, fuse_bit_size, SILICON_ID_SILICON_MAJOR_REVISION_WIDTH);
    will_return_always(__wrap_fuse_read, 1);
    platform_read_for_fuse((uintptr_t)&fuse_data, SILICON_ID_SILICON_MAJOR_REVISION_BIT_OFFSET, SILICON_ID_SILICON_MAJOR_REVISION_WIDTH);
}

TEST_FUNCTION(test_fuse_distribute_SIM_RTL, NULL, NULL)
{

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RTL_SIM);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);
    int status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP);
    assert_int_equal(status, SILIBS_E_SUPPORT);
    // Debug prints
}

TEST_FUNCTION(test_fuse_distribution_SVP_PRE_MESH, NULL, NULL)
{
    fuse_dist_exclude_range_t fuse_dist_exclude_list1[10] = {}; // Allocate memory
    uint32_t exclude_list_count1 = 10;                          // Match the size of fuse_dist_exclude_list1

    // Initialize the exclusion list with valid data
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        fuse_dist_exclude_list1[i].start_addr = i * 10;
        fuse_dist_exclude_list1[i].end_addr = i * 10 + 5;
    }

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);
    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);
    // Setup expectations for __wrap_fuse_dist_get_exclusion_list_secure
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, die_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, plat_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_list);
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_count);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    expect_function_call(__wrap_fuse_dist_get_exclusion_list_secure);

    // Verify exclusion list before setting expectations
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        printf("Exclusion list [%u]: start_addr=%llu, end_addr=%llu\n",
               i,
               fuse_dist_exclude_list1[i].start_addr,
               fuse_dist_exclude_list1[i].end_addr);
    }

    expect_any(__wrap_fuse_distribution, die_id);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_HSP_DIST_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, "
           "phase_min=POST_HSP_DIST_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    unsigned int status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP);
    assert_int_equal(status, 0);
    // Debug prints
    printf("Freed memory for fuse_dist_exclude_list1\n");
}

TEST_FUNCTION(test_fuse_distribution_SVP_POST_MESH, NULL, NULL)
{
    fuse_dist_exclude_range_t fuse_dist_exclude_list1[10] = {}; // Allocate memory
    uint32_t exclude_list_count1 = 10;                          // Match the size of fuse_dist_exclude_list1

    // Initialize the exclusion list with valid data
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        fuse_dist_exclude_list1[i].start_addr = i * 10;
        fuse_dist_exclude_list1[i].end_addr = i * 10 + 5;
    }

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_SVP_SIM);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);

    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);

    // Setup expectations for __wrap_fuse_dist_get_exclusion_list_secure
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, die_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, plat_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_list);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_count);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    expect_function_call_any(__wrap_fuse_dist_get_exclusion_list_secure);

    // Verify exclusion list before setting expectations
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        printf("Exclusion list [%u]: start_addr=%llu, end_addr=%llu\n",
               i,
               fuse_dist_exclude_list1[i].start_addr,
               fuse_dist_exclude_list1[i].end_addr);
    }

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf(
        "Before fuse_distribution: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, phase_min=MESH_INIT_MINOR, "
        "exclude_list=%p, exclude_list_count=%u\n",
        (void*)fuse_dist_exclude_list1,
        exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_MESH_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_BRIDGE_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_BRIDGE_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    unsigned int status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);
    assert_int_equal(status, 0);
    // Debug prints
    printf("Freed memory for fuse_dist_exclude_list1\n");
}

TEST_FUNCTION(test_fuse_distribute_FPGA_LARGE_0, NULL, NULL)
{
    fuse_dist_exclude_range_t fuse_dist_exclude_list1[10] = {}; // Allocate memory
    uint32_t exclude_list_count1 = 10;                          // Match the size of fuse_dist_exclude_list1

    // Initialize the exclusion list with valid data
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        fuse_dist_exclude_list1[i].start_addr = i * 10;
        fuse_dist_exclude_list1[i].end_addr = i * 10 + 5;
    }

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);
    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);

    // Setup expectations for __wrap_fuse_dist_get_exclusion_list_secure
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, die_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, plat_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_list);
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_count);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    expect_function_call(__wrap_fuse_dist_get_exclusion_list_secure);
    // Debug prints
    printf("Setup expectations for __wrap_fuse_get_exclude_list_soc_nano\n");

    // Verify exclusion list before setting expectations
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        printf("Exclusion list [%u]: start_addr=%llu, end_addr=%llu\n",
               i,
               fuse_dist_exclude_list1[i].start_addr,
               fuse_dist_exclude_list1[i].end_addr);
    }

    // TODO: reinstate with // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2002810
    expect_any(__wrap_fuse_distribution, die_id);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_HSP_DIST_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, "
           "phase_min=POST_HSP_DIST_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    unsigned int status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP);
    assert_int_equal(status, 0);
    // Debug prints
    printf("Freed memory for fuse_dist_exclude_list1\n");
}

TEST_FUNCTION(test_fuse_distribute_FPGA_LARGE_1, NULL, NULL)
{
    fuse_dist_exclude_range_t fuse_dist_exclude_list1[10] = {}; // Allocate memory
    uint32_t exclude_list_count1 = 10;                          // Match the size of fuse_dist_exclude_list1

    // Initialize the exclusion list with valid data
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        fuse_dist_exclude_list1[i].start_addr = i * 10;
        fuse_dist_exclude_list1[i].end_addr = i * 10 + 5;
    }

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_FPGA_LARGE);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);

    // Debug prints
    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);

    // Setup expectations for __wrap_fuse_dist_get_exclusion_list_secure
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, die_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, plat_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_list);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_count);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    expect_function_call_any(__wrap_fuse_dist_get_exclusion_list_secure);
    // Debug prints
    printf("Setup expectations for __wrap_fuse_get_exclude_list_soc_nano\n");

    // Verify exclusion list before setting expectations
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        printf("Exclusion list [%u]: start_addr=%llu, end_addr=%llu\n",
               i,
               fuse_dist_exclude_list1[i].start_addr,
               fuse_dist_exclude_list1[i].end_addr);
    }

    // TODO: reinstate with // https://azurecsi.visualstudio.com/Dev/_workitems/edit/2002810
    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf(
        "Before fuse_distribution: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, phase_min=MESH_INIT_MINOR, "
        "exclude_list=%p, exclude_list_count=%u\n",
        (void*)fuse_dist_exclude_list1,
        exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_MESH_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_BRIDGE_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_BRIDGE_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    unsigned int status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);
    assert_int_equal(status, 0);
    // Debug prints
    printf("Freed memory for fuse_dist_exclude_list1\n");
}

TEST_FUNCTION(test_fuse_distribution_emulation_PRE_MESH, NULL, NULL)
{
    fuse_dist_exclude_range_t fuse_dist_exclude_list1[10] = {}; // Allocate memory
    uint32_t exclude_list_count1 = 10;                          // Match the size of fuse_dist_exclude_list1

    // Initialize the exclusion list with valid data
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        fuse_dist_exclude_list1[i].start_addr = i * 10;
        fuse_dist_exclude_list1[i].end_addr = i * 10 + 5;
    }

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);
    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);
    // Setup expectations for __wrap_fuse_dist_get_exclusion_list_secure
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, die_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, plat_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_list);
    expect_any(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_count);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    expect_function_call(__wrap_fuse_dist_get_exclusion_list_secure);

    // Verify exclusion list before setting expectations
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        printf("Exclusion list [%u]: start_addr=%llu, end_addr=%llu\n",
               i,
               fuse_dist_exclude_list1[i].start_addr,
               fuse_dist_exclude_list1[i].end_addr);
    }

    expect_any(__wrap_fuse_distribution, die_id);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_HSP_DIST_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, "
           "phase_min=POST_HSP_DIST_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    unsigned int status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP);
    assert_int_equal(status, 0);
    // Debug prints
    printf("Freed memory for fuse_dist_exclude_list1\n");
}

TEST_FUNCTION(test_fuse_distribution_emulation_POST_MESH, NULL, NULL)
{
    fuse_dist_exclude_range_t fuse_dist_exclude_list1[10] = {}; // Allocate memory
    uint32_t exclude_list_count1 = 10;                          // Match the size of fuse_dist_exclude_list1

    // Initialize the exclusion list with valid data
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        fuse_dist_exclude_list1[i].start_addr = i * 10;
        fuse_dist_exclude_list1[i].end_addr = i * 10 + 5;
    }

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_EMU);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);

    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);

    // Setup expectations for __wrap_fuse_dist_get_exclusion_list_secure
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, die_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, plat_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_list);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_count);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    expect_function_call_any(__wrap_fuse_dist_get_exclusion_list_secure);

    // Verify exclusion list before setting expectations
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        printf("Exclusion list [%u]: start_addr=%llu, end_addr=%llu\n",
               i,
               fuse_dist_exclude_list1[i].start_addr,
               fuse_dist_exclude_list1[i].end_addr);
    }

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf(
        "Before fuse_distribution: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, phase_min=MESH_INIT_MINOR, "
        "exclude_list=%p, exclude_list_count=%u\n",
        (void*)fuse_dist_exclude_list1,
        exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_MESH_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_BRIDGE_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_BRIDGE_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    unsigned int status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);
    assert_int_equal(status, 0);
    // Debug prints
    printf("Freed memory for fuse_dist_exclude_list1\n");
}

TEST_FUNCTION(test_fuse_distribution_soc_POST_MESH, NULL, NULL)
{
    fuse_dist_exclude_range_t fuse_dist_exclude_list1[10] = {}; // Allocate memory
    uint32_t exclude_list_count1 = 10;                          // Match the size of fuse_dist_exclude_list1

    // Initialize the exclusion list with valid data
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        fuse_dist_exclude_list1[i].start_addr = i * 10;
        fuse_dist_exclude_list1[i].end_addr = i * 10 + 5;
    }

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);

    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);

    // Setup expectations for __wrap_fuse_dist_get_exclusion_list_secure
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, die_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, plat_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_list);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_count);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, false);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    expect_function_call_any(__wrap_fuse_dist_get_exclusion_list_secure);

    // Verify exclusion list before setting expectations
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        printf("Exclusion list [%u]: start_addr=%llu, end_addr=%llu\n",
               i,
               fuse_dist_exclude_list1[i].start_addr,
               fuse_dist_exclude_list1[i].end_addr);
    }

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf(
        "Before fuse_distribution: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, phase_min=MESH_INIT_MINOR, "
        "exclude_list=%p, exclude_list_count=%u\n",
        (void*)fuse_dist_exclude_list1,
        exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_MESH_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_BRIDGE_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_BRIDGE_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    unsigned int status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);
    assert_int_equal(status, 0);
    // Debug prints
    printf("Freed memory for fuse_dist_exclude_list1\n");
}

TEST_FUNCTION(test_fuse_distribution_soc_POST_MESH_secure, NULL, NULL)
{
    fuse_dist_exclude_range_t fuse_dist_exclude_list1[10] = {}; // Allocate memory
    uint32_t exclude_list_count1 = 10;                          // Match the size of fuse_dist_exclude_list1

    // Initialize the exclusion list with valid data
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        fuse_dist_exclude_list1[i].start_addr = i * 10;
        fuse_dist_exclude_list1[i].end_addr = i * 10 + 5;
    }

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_SECURE);

    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);

    // Setup expectations for __wrap_fuse_dist_get_exclusion_list_secure
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, die_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, plat_id);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_list);
    expect_any_always(__wrap_fuse_dist_get_exclusion_list_secure, IP_exclude_count);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, true);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, true);
    expect_value(__wrap_fuse_dist_get_exclusion_list_secure, is_secure, true);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list_secure, exclude_list_count1);
    expect_function_call_any(__wrap_fuse_dist_get_exclusion_list_secure);

    // Verify exclusion list before setting expectations
    for (uint32_t i = 0; i < exclude_list_count1; ++i)
    {
        printf("Exclusion list [%u]: start_addr=%llu, end_addr=%llu\n",
               i,
               fuse_dist_exclude_list1[i].start_addr,
               fuse_dist_exclude_list1[i].end_addr);
    }

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf(
        "Before fuse_distribution: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, phase_min=MESH_INIT_MINOR, "
        "exclude_list=%p, exclude_list_count=%u\n",
        (void*)fuse_dist_exclude_list1,
        exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_MESH_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_MESH_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    expect_value(__wrap_fuse_distribution, die_id, DIE_0);
    expect_value(__wrap_fuse_distribution, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_fuse_distribution, phase_min, POST_BRIDGE_INIT_MINOR);
    expect_memory(__wrap_fuse_distribution, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_fuse_distribution, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_fuse_distribution);

    printf("Before fuse_distribution: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_BRIDGE_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    unsigned int status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_HSP_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT);
    assert_int_equal(status, 0);
    status = platform_fuse_distribution(FUSE_DISTRIBUTION_STAGE_POST_MESH_INIT_BRIDGE_INIT);
    assert_int_equal(status, 0);
    // Debug prints
    printf("Freed memory for fuse_dist_exclude_list1\n");
}

static void mock_ap_core_die_cfg_cb(void* context)
{
    // This function is a placeholder for the actual implementation
    // that would handle the core die configuration.
    FPFW_UNUSED(context);
    function_called();
}

TEST_FUNCTION(test_fuse_core_to_ap_die0, NULL, NULL)
{
    // mocks
    kng_fuse_disable_core_t DIE0_core_disable_pre_knob_test = {0x0, 0x0, 0xfffffff0, 0xffffffff};
    kng_fuse_disable_core_t DIE0_core_disable_post_knob_test = {0x2, 0x0, 0xfffffff3, 0xffffffff};
    kng_hns_fuses_t DIE0_hns_fuses_test = {0x0};
    DFWK_ASYNC_REQUEST_HEADER dummy_request;

    // fuse_init mocks to get config_knobs
    fpfw_icc_base_ctx_t* dummy_icc_hspmbx_ctx = reinterpret_cast<fpfw_icc_base_ctx_t*>(1);
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    will_return(__wrap_config_get_die0_core_disable_value_31_0, 0x00000002);
    will_return(__wrap_config_get_die0_core_disable_value_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_95_64, 0x0000000F);

    will_return(__wrap_config_get_die0_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_95_64, 0x0000000C);

    fuse_init(dummy_icc_hspmbx_ctx);

    // Set fuses
    expect_memory(__wrap_read_core_defect_fuses,
                  fuse_dis_core_67_64,
                  &(DIE0_core_disable_pre_knob_test.fuse_dis_core_95_64),
                  sizeof(DIE0_core_disable_pre_knob_test.fuse_dis_core_95_64));
    expect_memory(__wrap_read_core_defect_fuses,
                  fuse_dis_core_63_32,
                  &(DIE0_core_disable_pre_knob_test.fuse_dis_core_63_32),
                  sizeof(DIE0_core_disable_pre_knob_test.fuse_dis_core_63_32));
    expect_memory(__wrap_read_core_defect_fuses,
                  fuse_dis_core_31_0,
                  &(DIE0_fuse_disable_test.fuse_dis_core_31_0),
                  sizeof(DIE0_core_disable_pre_knob_test.fuse_dis_core_31_0));
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);

    read_core_disables();

    will_return_always(__wrap_mesh_get_hns_sds_vector_from_hns_sparring, 0x0);
    expect_value(__wrap_sds_block_write, sds_module_id, FUSE_DISABLE_CORE_DIE0_STRUCT_ID);
    expect_memory(__wrap_sds_block_write, buffer, &(DIE0_core_disable_post_knob_test), FUSE_DISABLE_CORE_DIE0_SIZE);
    expect_value(__wrap_sds_block_write, buffer_size, FUSE_DISABLE_CORE_DIE0_SIZE);

    expect_value(__wrap_sds_block_write, sds_module_id, HNS_FUSES_DIE0_STRUCT_ID);
    expect_memory(__wrap_sds_block_write, buffer, &(DIE0_hns_fuses_test), HNS_FUSES_SIZE);
    expect_value(__wrap_sds_block_write, buffer_size, HNS_FUSES_SIZE);

    register_remote_die_cfg_completion_cb(mock_ap_core_die_cfg_cb, &dummy_request);
    expect_function_call(mock_ap_core_die_cfg_cb);
    write_fuse_info_to_ap();
}

TEST_FUNCTION(test_fuse_core_ap_die1, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER dummy_request;
    register_remote_die_cfg_completion_cb(mock_ap_core_die_cfg_cb, &dummy_request);
    will_return_always(__wrap_mesh_get_hns_sds_vector_from_hns_sparring, 0x0);

    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return_always(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_icc_base_send);

    expect_function_call(mock_ap_core_die_cfg_cb);
    write_fuse_info_to_ap();
}

// Negative Test
TEST_FUNCTION(test_fuse_core_ap_sparring_die0, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER dummy_request;
    kng_fuse_disable_core_t DIE0_core_disable_post_knob_test = {0x2, 0x0, 0xfffffff3, 0xffffffff};

    register_remote_die_cfg_completion_cb(mock_ap_core_die_cfg_cb, &dummy_request);
    will_return_always(__wrap_mesh_get_hns_sds_vector_from_hns_sparring, -1);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    expect_value(__wrap_sds_block_write, sds_module_id, FUSE_DISABLE_CORE_DIE0_STRUCT_ID);
    expect_memory(__wrap_sds_block_write, buffer, &(DIE0_core_disable_post_knob_test), FUSE_DISABLE_CORE_DIE0_SIZE);
    expect_value(__wrap_sds_block_write, buffer_size, FUSE_DISABLE_CORE_DIE0_SIZE);

    int status = write_fuse_info_to_ap();
    assert_int_equal(status, -1);
}

// Negative Test
TEST_FUNCTION(test_fuse_core_ap_sparring_die1, NULL, NULL)
{
    DFWK_ASYNC_REQUEST_HEADER dummy_request;
    register_remote_die_cfg_completion_cb(mock_ap_core_die_cfg_cb, &dummy_request);
    will_return_always(__wrap_mesh_get_hns_sds_vector_from_hns_sparring, -1);

    will_return_always(__wrap_idsw_get_die_id, DIE_1);

    int status = write_fuse_info_to_ap();
    assert_int_equal(status, -1);
}

TEST_FUNCTION(test_fuse_save_remote_die_config, NULL, NULL)
{
    // mocks
    DFWK_ASYNC_REQUEST_HEADER dummy_request;
    register_remote_die_cfg_completion_cb(mock_ap_core_die_cfg_cb, &dummy_request);
    kng_hns_fuses_t DIE1_hns_fuses_test = {0x0};
    kng_fuse_disable_core_t expected = DIE1_core_disable_post_knob_test;
    expect_value(__wrap_sds_block_write, sds_module_id, FUSE_DISABLE_CORE_DIE1_STRUCT_ID);
    expect_memory(__wrap_sds_block_write, buffer, &expected, FUSE_DISABLE_CORE_DIE1_SIZE);
    expect_value(__wrap_sds_block_write, buffer_size, FUSE_DISABLE_CORE_DIE1_SIZE);

    expect_value(__wrap_sds_block_write, sds_module_id, HNS_FUSES_DIE1_STRUCT_ID);
    expect_memory(__wrap_sds_block_write, buffer, &(DIE1_hns_fuses_test), HNS_FUSES_SIZE);
    expect_value(__wrap_sds_block_write, buffer_size, HNS_FUSES_SIZE);

    static uint8_t icc_mhu_send_die2die_payload[512] = {0};
    icc_mhu_d2d_fuse_packet_t* die2die_send_msg = (icc_mhu_d2d_fuse_packet_t*)icc_mhu_send_die2die_payload;
    die2die_send_msg->payload[0] = DIE1_core_disable_post_knob_test.fuse_dis_core_31_0;
    die2die_send_msg->payload[1] = DIE1_core_disable_post_knob_test.fuse_dis_core_63_32;
    die2die_send_msg->payload[2] = DIE1_core_disable_post_knob_test.fuse_dis_core_95_64;
    die2die_send_msg->payload[3] = DIE1_core_disable_post_knob_test.fuse_dis_core_127_96;
    die2die_send_msg->payload[4] = DIE1_hns_fuses_test.hns_fuses_31_0;
    die2die_send_msg->payload[5] = DIE1_hns_fuses_test.hns_fuses_63_32;
    die2die_send_msg->payload[6] = DIE1_hns_fuses_test.hns_fuses_95_64;

    expect_function_call(mock_ap_core_die_cfg_cb);
    save_remote_die_config_cb(&icc_mhu_send_die2die_payload, 0, 0);
}

TEST_FUNCTION(test_fuse_prepare_remote_die_config_listner, NULL, NULL)
{

    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, ICC_SSI_STAGE_SDS_DIE_CFG);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_ICC_BASE_STATUS_SUCCESS);
    prepare_remote_die_config_listener((fpfw_icc_base_ctx_t*)1234);
}

TEST_FUNCTION(test_fuse_distribute_bug_assert, NULL, NULL)
{
    // mocks
    kng_fuse_disable_core_t DIE0_core_disable_post_knob_test = {0x02, 0x00, 0xFFFFFFF3, 0xFFFFFFFF};

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RVP_EVT_SILICON);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_system_info_get_security_state, HSP_SECURITY_STATE_TEST);

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    expect_value(__wrap_fuse_read, fuse_bit_offset, SILICON_ID_SILICON_MAJOR_REVISION_BIT_OFFSET);
    expect_value(__wrap_fuse_read, fuse_bit_size, SILICON_ID_SILICON_MAJOR_REVISION_WIDTH);
    will_return(__wrap_fuse_read, 0);
    will_return(__wrap_fuse_dma_copy_to_ram_blocking, 1);

    // Expectation for fuse_override
    expect_value(__wrap_fuse_override_ignoring_valids, die_id, DIE_0);
    expect_value(__wrap_fuse_override_ignoring_valids, override_buffer, (uintptr_t)(SCP_EXP_FUSE_DATA_BASE));
    expect_function_call(__wrap_fuse_override_ignoring_valids);

    will_return(__wrap_system_info_is_warm_start, false);
    expect_memory(__wrap_read_core_defect_fuses,
                  fuse_dis_core_67_64,
                  &(DIE0_core_disable_post_knob_test.fuse_dis_core_95_64),
                  sizeof(DIE0_core_disable_post_knob_test.fuse_dis_core_95_64));
    expect_memory(__wrap_read_core_defect_fuses,
                  fuse_dis_core_63_32,
                  &(DIE0_core_disable_post_knob_test.fuse_dis_core_63_32),
                  sizeof(DIE0_core_disable_post_knob_test.fuse_dis_core_63_32));
    expect_memory(__wrap_read_core_defect_fuses,
                  fuse_dis_core_31_0,
                  &(DIE0_core_disable_post_knob_test.fuse_dis_core_31_0),
                  sizeof(DIE0_core_disable_post_knob_test.fuse_dis_core_31_0));
    will_return(__wrap_read_core_defect_fuses, SILIBS_SUCCESS);
    // Expectation for trigger_debugger_for_manual_overrides
    expect_function_call(__wrap_trigger_debugger_for_manual_overrides);
    if (!BUGCHECK_MOCK_RETURN())
    {
        platform_fuse_override();
    }

    will_return(__wrap_fuse_dma_copy_to_ram_blocking, 0);

    if (!BUGCHECK_MOCK_RETURN())
    {
        platform_fuse_override();
    }
}
}

TEST_FUNCTION(test_fuse_init_single_die, NULL, NULL)
{
    fpfw_icc_base_ctx_t* dummy_icc_hspmbx_ctx = reinterpret_cast<fpfw_icc_base_ctx_t*>(1);

    will_return(__wrap_idsw_get_die_id, DIE_0);

    will_return(__wrap_config_get_die0_core_disable_value_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_disable_value_63_32, 0x00000001);
    will_return(__wrap_config_get_die0_core_disable_value_95_64, 0x00000003);

    will_return(__wrap_config_get_die0_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_63_32, 0x00000000);
    will_return(__wrap_config_get_die0_core_spare_en_95_64, 0x00000002);

    fuse_init(dummy_icc_hspmbx_ctx);
}

TEST_FUNCTION(test_fuse_init_dual_die, NULL, NULL)
{
    fpfw_icc_base_ctx_t* dummy_icc_hspmbx_ctx = reinterpret_cast<fpfw_icc_base_ctx_t*>(1);

    will_return_always(__wrap_idsw_get_die_id, DIE_1);
    will_return(__wrap_config_get_die1_core_disable_value_31_0, 0x00000000);
    will_return(__wrap_config_get_die1_core_disable_value_63_32, 0x00000001);
    will_return(__wrap_config_get_die1_core_disable_value_95_64, 0x00000003);

    will_return(__wrap_config_get_die1_core_spare_en_31_0, 0x00000000);
    will_return(__wrap_config_get_die1_core_spare_en_63_32, 0x00000000);
    will_return(__wrap_config_get_die1_core_spare_en_95_64, 0x00000002);

    fuse_init(dummy_icc_hspmbx_ctx);
}

TEST_FUNCTION(test_fuse_disable_cores_to_66_total_zero, NULL, NULL)
{
    // Arrange
    kng_fuse_disable_core_t fuse = {0}; // all fields 0
    will_return(__wrap_idsw_get_die_id, DIE_0);

    // Act
    fuse_disable_cores_to_66(&fuse);

    // Assert
    // Bit 26 of fuse_dis_core_31_0 should be set
    assert_true(fuse.fuse_dis_core_31_0 & (1u << 26));
    // Bit 6 of fuse_dis_core_63_32 should be set
    assert_true(fuse.fuse_dis_core_63_32 & (1u << 5));
}

// Simple test when exactly one core is disabled (no change to struct)
TEST_FUNCTION(test_fuse_disable_cores_to_66_total_two, NULL, NULL)
{
    // Arrange
    kng_fuse_disable_core_t fuse = {0};
    will_return(__wrap_idsw_get_die_id, DIE_0);
    // Disable only bit 3 in first word (simulate 1 core disabled)
    fuse.fuse_dis_core_31_0 = (1u << 3);

    // Save a copy
    kng_fuse_disable_core_t before = fuse;

    // Act
    fuse_disable_cores_to_66(&fuse);

    // Assert
    // core”0 count=1 still triggers slot1 => bit 6 in the 32–63 word
    assert_int_equal(fuse.fuse_dis_core_31_0, before.fuse_dis_core_31_0);
    assert_true(fuse.fuse_dis_core_63_32 & (1u << 5));
    assert_int_equal(fuse.fuse_dis_core_95_64, before.fuse_dis_core_95_64);
}
TEST_FUNCTION(test_is_core_disabled, NULL, NULL)
{
    kng_fuse_disable_core_t fuse = {0};

    //
    fuse.fuse_dis_core_31_0 = (1U << (5));
    assert_true(is_core_disabled(&fuse, 5));
    fuse.fuse_dis_core_31_0 = 0;
    fuse.fuse_dis_core_63_32 = (1u << (40 - 32));
    assert_true(is_core_disabled(&fuse, 40));
    fuse.fuse_dis_core_31_0 = 0;
    fuse.fuse_dis_core_63_32 = 0;
    fuse.fuse_dis_core_95_64 = (1u << (66 - 64));
    assert_true(is_core_disabled(&fuse, 66));
    assert_false(is_core_disabled(&fuse, 70));

    assert_false(is_core_disabled(&fuse, 6));
}

//
// disable_core should set the correct bit in the proper word
//
TEST_FUNCTION(test_disable_core, NULL, NULL)
{
    kng_fuse_disable_core_t fuse = {0};

    // Disable core 10 (word0)
    disable_core(&fuse, 10);
    assert_true(fuse.fuse_dis_core_31_0 & (1U << 10));

    // Disable core 33 (word1)
    disable_core(&fuse, 33);
    assert_true(fuse.fuse_dis_core_63_32 & (1U << (33 - 32)));

    // Disable core 65 (word2)
    disable_core(&fuse, 65);
    assert_true(fuse.fuse_dis_core_95_64 & (1U << (65 - 64)));
}

//
// fuse_disable_pick_algorithm should pick the first valid priority core (37)
//
TEST_FUNCTION(test_fuse_disable_pick_algorithm_default, NULL, NULL)
{
    kng_fuse_disable_core_t fuse = {0};

    fuse_disable_pick_algorithm(&fuse);

    assert_true(fuse.fuse_dis_core_63_32 & (1U << (37 - 32)));
}
TEST_FUNCTION(test_fuse_disable_pick_algorithm_skip_self, NULL, NULL)
{
    kng_fuse_disable_core_t fuse = {0};

    disable_core(&fuse, 37);

    fuse_disable_pick_algorithm(&fuse);

    assert_true(is_core_disabled(&fuse, 26));
}

TEST_FUNCTION(test_fuse_disable_pick_algorithm_column_block, NULL, NULL)
{
    kng_fuse_disable_core_t fuse = {0};

    disable_core(&fuse, 32);

    fuse_disable_pick_algorithm(&fuse);

    assert_true(is_core_disabled(&fuse, 49));
}

TEST_FUNCTION(test_fuse_disable_pick_algorithm_row_block, NULL, NULL)
{
    kng_fuse_disable_core_t fuse = {0};

    disable_core(&fuse, 25);
    fuse_disable_pick_algorithm(&fuse);
    assert_true(is_core_disabled(&fuse, 38));
}

TEST_FUNCTION(test_fuse_post_mesh_init, NULL, NULL)
{
    will_return_always(__wrap_idhw_is_single_die_boot_en, true);
    fpfw_icc_base_ctx_t* dummy_icc_d2d_ctx = reinterpret_cast<fpfw_icc_base_ctx_t*>(1);

    fuse_post_mesh_init(dummy_icc_d2d_ctx);
}
