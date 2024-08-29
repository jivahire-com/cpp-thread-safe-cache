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
#include <FpFwUtils.h>     // for FPFW_UNUSED
#include <fpfw_icc_base.h> // for fpfw_icc_base_send_recv_req_t, fpfw...
#include <fpfw_init.h>     // for fpfw_init_get_handle, FPFW_INIT_C...
#include <fpfw_status.h>   // for fpfw_status_t
#include <fuse.h>
#include <fuse_client.h>
#include <fuse_dist_platform_exclusions.h>
#include <fuse_init.h>
#include <fuse_struct.h>
#include <fuses_top_regs.h>
#include <hsp_firmware_headers.h>
#include <idsw.h>
#include <idsw_kng.h>
#include <kingsgate_fuse_defines.h> // Test revision get
#include <setjmp.h>
#include <silibs_platform.h>
#include <silibs_scp_exp_top_regs.h>
#include <silibs_scp_top_regs.h>
#include <stdnoreturn.h>
#include <utils.h> // for UNUSED

/*-- Symbolic Constant Macros (defines) --*/
#define BUGCHECK_MOCK_RETURN() (setjmp(cd_mock_jump_buf))

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
// bool simulate_fuse = true;
static icc_base_recv_complete_notify fw_load_cb = NULL;
static jmp_buf cd_mock_jump_buf;
static uint32_t icc_hspmbx_ctx;

/*------------- Functions ----------------*/
extern int read_override_from_spi();
extern bool platform_requires_fuse_distribution();
extern int platform_fuse_copy_to_ram();
extern void fuse_icc_base_recv_complete_notify(void* context, size_t output_size_bytes, fpfw_status_t status);
extern void request_load_fuse_send_complete_cb(void* context, fpfw_status_t status);
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

silibs_status_t __wrap_fuse_dma_copy_to_ram_blocking()
{
    // function_called();
    return mock_type(int);
}

int __wrap_read_fuse(const unsigned fuse_bit_offset, const unsigned fuse_bit_size)
{
    check_expected(fuse_bit_offset);
    check_expected(fuse_bit_size);
    // function_called();
    return mock_type(int);
}

silibs_status_t __wrap_apply_fuse_override(KNG_DIE_ID die_id, const uintptr_t override_buffer)
{
    check_expected(die_id);
    check_expected_ptr(override_buffer);
    function_called();
    return SILIBS_SUCCESS;
}

silibs_status_t __wrap_apply_fuse_override_ignoring_valids(idsw_die_id_t die_id, const uintptr_t override_buffer)
{
    check_expected(die_id);
    check_expected_ptr(override_buffer);
    function_called();
    return 0;
}

silibs_status_t __wrap_distribute_fuses(KNG_DIE_ID die_id,
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
    printf("In __wrap_distribute_fuses:\n");
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

silibs_status_t __wrap_fuse_dist_get_exclusion_list(KNG_DIE_ID die_id,
                                                    KNG_PLAT_ID plat_id,
                                                    const fuse_dist_exclude_range_t** IP_exclude_list,
                                                    uint32_t* IP_exclude_count)
{
    check_expected(die_id);
    check_expected(plat_id);
    check_expected_ptr(IP_exclude_list);
    check_expected_ptr(IP_exclude_count);
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
fpfw_status_t __wrap_fpfw_icc_base_send(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_send_req_t* params)
{
    assert_non_null(icc_ctx);
    check_expected_ptr(params->payload_buffer);
    return mock_type(fpfw_status_t);
}

fpfw_status_t __wrap_fpfw_icc_base_recv(fpfw_icc_base_ctx_t* icc_ctx, fpfw_icc_base_recv_req_t* params)
{
    assert_non_null(icc_ctx);
    check_expected(params->recv_cmd_code);
    fw_load_cb = params->cb;
    ((kng_hsp_mailbox_msg*)(params->payload_buffer))->header.cmd = mock_type(int);

    return mock_type(fpfw_status_t);
}
//
// Tests
//
// test fuse_read
TEST_FUNCTION(test_fuse_init, NULL, NULL)
{

    fuse_init((fpfw_icc_base_ctx_t*)&icc_hspmbx_ctx);
}

TEST_FUNCTION(test_fuse_override_SIM, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RTL_SIM);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    will_return_always(__wrap_fuse_dma_copy_to_ram_blocking, 0);
    expect_value(__wrap_read_fuse, fuse_bit_offset, TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_BIT_OFFSET);
    expect_value(__wrap_read_fuse, fuse_bit_size, TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_WIDTH);
    will_return_always(__wrap_read_fuse, 1);
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_REQ,
        .header.context = 0,
        .id = HspFirmwareIdFuseOverrideDie0,
        .address = SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS,
        .size = SCP_EXP_TOP_RAM1_SIZE,
    };
    expect_memory(__wrap_fpfw_icc_base_send, params->payload_buffer, &send_request, sizeof(send_request));
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);

    // Expectation for apply_fuse_override
    expect_value(__wrap_apply_fuse_override, die_id, DIE_0);
    expect_value(__wrap_apply_fuse_override, override_buffer, (uintptr_t)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS));
    expect_function_call(__wrap_apply_fuse_override);

    // Expectation for trigger_debugger_for_manual_overrides
    expect_function_call(__wrap_trigger_debugger_for_manual_overrides);
    assert_int_equal(platform_fuse_override(), 0);
}

TEST_FUNCTION(test_read_fuse, NULL, NULL)
{
    uint64_t fuse_data = 0;
    expect_value(__wrap_read_fuse, fuse_bit_offset, TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_BIT_OFFSET);
    expect_value(__wrap_read_fuse, fuse_bit_size, TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_WIDTH);
    will_return_always(__wrap_read_fuse, 1);
    platform_read_for_fuse((uintptr_t)&fuse_data,
                           TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_BIT_OFFSET,
                           TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_WIDTH);
}

TEST_FUNCTION(test_fuse_distribute_SIM_RTL, NULL, NULL)
{

    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RTL_SIM);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);
    int status = platform_fuse_distribution(0);
    assert_int_equal(status, SILIBS_E_SUPPORT);
    // Debug prints
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
    // Debug prints
    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);

    // Setup expectations for __wrap_fuse_dist_get_exclusion_list
    expect_any(__wrap_fuse_dist_get_exclusion_list, die_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list, plat_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list, IP_exclude_list);
    expect_any(__wrap_fuse_dist_get_exclusion_list, IP_exclude_count);
    will_return(__wrap_fuse_dist_get_exclusion_list, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list, exclude_list_count1);
    expect_function_call(__wrap_fuse_dist_get_exclusion_list);
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

    // Setup expectations for __wrap_distribute_fuses
    expect_any(__wrap_distribute_fuses, die_id);
    expect_value(__wrap_distribute_fuses, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_distribute_fuses, phase_min, POST_HSP_DIST_MINOR);
    expect_memory(__wrap_distribute_fuses, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_distribute_fuses, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_distribute_fuses);

    printf("Before distribute_fuses: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, "
           "phase_min=POST_HSP_DIST_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);
    
    unsigned int status = platform_fuse_distribution(0);
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
    // Debug prints
    // Debug prints
    printf("Allocated memory for fuse_dist_exclude_list1 at %p\n", (void*)fuse_dist_exclude_list1);

    // Setup expectations for __wrap_fuse_dist_get_exclusion_list
    expect_any(__wrap_fuse_dist_get_exclusion_list, die_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list, plat_id);
    expect_any(__wrap_fuse_dist_get_exclusion_list, IP_exclude_list);
    expect_any(__wrap_fuse_dist_get_exclusion_list, IP_exclude_count);
    will_return(__wrap_fuse_dist_get_exclusion_list, fuse_dist_exclude_list1);
    will_return(__wrap_fuse_dist_get_exclusion_list, exclude_list_count1);
    expect_function_call(__wrap_fuse_dist_get_exclusion_list);
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

    // Setup expectations for __wrap_distribute_fuses

    
    expect_value(__wrap_distribute_fuses, die_id, DIE_0);
    expect_value(__wrap_distribute_fuses, phase_maj, POST_HSP_DIST_MAJOR);
    expect_value(__wrap_distribute_fuses, phase_min, MESH_INIT_MINOR);
    expect_memory(__wrap_distribute_fuses, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_distribute_fuses, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_distribute_fuses);

    printf("Before distribute_fuses: die_id=DIE_0, phase_maj=POST_HSP_DIST_MAJOR, phase_min=MESH_INIT_MINOR, "
           "exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    expect_value(__wrap_distribute_fuses, die_id, DIE_0);
    expect_value(__wrap_distribute_fuses, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_distribute_fuses, phase_min, POST_MESH_INIT_MINOR);
    expect_memory(__wrap_distribute_fuses, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_distribute_fuses, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_distribute_fuses);

    printf("Before distribute_fuses: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_MESH_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    expect_value(__wrap_distribute_fuses, die_id, DIE_0);
    expect_value(__wrap_distribute_fuses, phase_maj, POST_MESH_INIT_MAJOR);
    expect_value(__wrap_distribute_fuses, phase_min, POST_BRIDGE_INIT_MINOR);
    expect_memory(__wrap_distribute_fuses, exclude_list, fuse_dist_exclude_list1, sizeof(fuse_dist_exclude_range_t) * exclude_list_count1);
    expect_value(__wrap_distribute_fuses, exclude_list_count, exclude_list_count1);
    expect_function_call(__wrap_distribute_fuses);

    printf("Before distribute_fuses: die_id=DIE_0, phase_maj=POST_MESH_INIT_MAJOR, "
           "phase_min=POST_BRIDGE_INIT_MINOR, exclude_list=%p, exclude_list_count=%u\n",
           (void*)fuse_dist_exclude_list1,
           exclude_list_count1);

    unsigned int status = platform_fuse_distribution(1);
    assert_int_equal(status, 0);
    // Debug prints
    printf("Freed memory for fuse_dist_exclude_list1\n");
}

TEST_FUNCTION(test_fuse_distribute_bug_assert, NULL, NULL)
{
    will_return_always(__wrap_idsw_get_platform_sdv, PLATFORM_RTL_SIM);
    will_return_always(__wrap_idsw_get_die_id, DIE_0);

    expect_any_always(__wrap_crash_dump_bug_check, errorCode);
    expect_value(__wrap_read_fuse, fuse_bit_offset, TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_BIT_OFFSET);
    expect_value(__wrap_read_fuse, fuse_bit_size, TEST_FLOW_CHECKS_SILICON_MAJOR_REVISION_WIDTH);
    will_return(__wrap_fuse_dma_copy_to_ram_blocking, 1);

    // Mock ICC send/receive function
    expect_value(__wrap_fpfw_icc_base_recv, params->recv_cmd_code, HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_RSP);
    will_return(__wrap_fpfw_icc_base_recv, HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_RSP);
    will_return(__wrap_fpfw_icc_base_recv, FPFW_STATUS_SUCCESS);

    kng_hsp_mailbox_cmd_load_fw_req send_request = {
        .header.cmd = HSP_MAILBOX_MSG_FUSE_AND_IMAGE_LOAD_REQ,
        .header.context = 0,
        .id = HspFirmwareIdFuseOverrideDie0,
        .address = SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM1_ADDRESS,
        .size = SCP_EXP_TOP_RAM1_SIZE,
    };
    expect_memory(__wrap_fpfw_icc_base_send, params->payload_buffer, &send_request, sizeof(send_request));
    will_return(__wrap_fpfw_icc_base_send, FPFW_ICC_BASE_STATUS_SUCCESS);
    // Expectation for apply_fuse_override
    expect_value(__wrap_apply_fuse_override_ignoring_valids, die_id, DIE_0);
    expect_value(__wrap_apply_fuse_override_ignoring_valids,
                 override_buffer,
                 (uintptr_t)(SCP_TOP_SCP_EXP_ADDRESS + SCP_EXP_TOP_RAM0_ADDRESS));
    expect_function_call(__wrap_apply_fuse_override_ignoring_valids);
    // Expectation for trigger_debugger_for_manual_overrides
    expect_function_call(__wrap_trigger_debugger_for_manual_overrides);
    if (!BUGCHECK_MOCK_RETURN())
    {
        platform_fuse_override();
    }

    will_return(__wrap_fuse_dma_copy_to_ram_blocking, 0);
    will_return_always(__wrap_read_fuse, 0);

    if (!BUGCHECK_MOCK_RETURN())
    {
        platform_fuse_override();
    }
}
}
