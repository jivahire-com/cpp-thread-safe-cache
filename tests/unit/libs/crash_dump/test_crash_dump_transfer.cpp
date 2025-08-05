//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_crash_dump.cpp
 * Crash dump unit tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for check_expected_ptr, expect_fun...
#include <cstdint>         // for uint32_t, uint64_t

extern "C" {
#include <../src/crash_dump_icc.h>
#include <../src/crash_dump_pldm.h>
#include <../src/crash_dump_stream.h>
#include <crash_dump.h>
#include <crash_dump_memory.h>
#include <fpfw_pldm_service.h>
#include <gpio_lib.h>
#include <idsw_kng.h>

/*-- Symbolic Constant Macros (defines) --*/
#define CRASH_DUMP_PAYLOAD_SIZE 512

/*------------- Typedefs -----------------*/
typedef struct
{
    DUMP_HEADER header;
    DUMP_METADATA_HEADER metadata_header;
    DUMP_PAYLOAD_HEADER payload_header;
    DUMP_CHUNK_DESCRIPTOR first_chunk_desc;
    uint8_t payload[CRASH_DUMP_PAYLOAD_SIZE];
} crash_dump_file_test_t;

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/
crash_dump_file_test_t crash_dump_region[2 * CRASH_DUMP_CORE_NUM] = {};

extern uint8_t* __real_get_crash_dump_region_address(atu_map_entry_t* die1_entry, KNG_DIE_ID die_id, crash_dump_core_t core_id);

/*------------- Functions ----------------*/
//
// Mocks
//
int __wrap_atu_map(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    assert_int_equal(atu_id, ATU_ID_MSCP);
    assert_non_null(atu_map_entry);

    return mock_type(int);
}

int __wrap_atu_unmap(atu_id_t atu_id, atu_map_entry_t* atu_map_entry)
{
    assert_int_equal(atu_id, ATU_ID_MSCP);
    assert_non_null(atu_map_entry);

    return mock_type(int);
}

uint8_t* __wrap_get_crash_dump_region_address(atu_map_entry_t* die1_entry, KNG_DIE_ID die_id, crash_dump_core_t core_id)
{
    assert_non_null(die1_entry);

    return (uint8_t*)&crash_dump_region[(int)die_id * CRASH_DUMP_CORE_NUM + (int)core_id];
}

fpfw_status_t __wrap_fpfw_pldm_service_raise_platform_event(pldm_platform_event_config_t* p_pe_config,
                                                            pldm_platform_event_notification* p_notification)
{
    assert_non_null(p_pe_config);
    assert_non_null(p_notification);

    function_called();

    return mock_type(fpfw_status_t);
}

void reset_crash_dump_region()
{
    memset(crash_dump_region, 0, sizeof(crash_dump_region));
}

void build_crash_dump_data(KNG_DIE_ID die_id, crash_dump_core_t core_id, uint32_t payload_size, uint8_t payload)
{
    crash_dump_file_test_t* dump = &crash_dump_region[(int)die_id * CRASH_DUMP_CORE_NUM + (int)core_id];

    dump->header.Magic = DUMP_HEADER_MAGIC_COMPLETE;
    dump->header.FileSize = sizeof(DUMP_HEADER) + sizeof(DUMP_METADATA_HEADER) + sizeof(DUMP_PAYLOAD_HEADER) + payload_size;
    dump->header.MetadataExtent.Offset = sizeof(DUMP_HEADER);
    dump->header.MetadataExtent.Size = sizeof(DUMP_METADATA_HEADER);
    dump->header.PayloadExtent.Offset = sizeof(DUMP_HEADER) + sizeof(DUMP_METADATA_HEADER);
    dump->header.PayloadExtent.Size = sizeof(DUMP_PAYLOAD_HEADER) + payload_size;
    dump->metadata_header.Magic = DUMP_METADATA_MAGIC;
    dump->metadata_header.Root.ChunkDescriptor.Magic = DUMP_CHUNK_MAGIC_FINALIZED;
    dump->metadata_header.Root.ChunkDescriptor.Type = DumpChunkType_Aggregate;
    dump->metadata_header.Root.ChunkDescriptor.HeaderSize = sizeof(DUMP_AGGREGATE_DESCRIPTOR);
    dump->metadata_header.Root.ChunkDescriptor.PayloadSize = 0;
    dump->metadata_header.Root.ChunkCount = 0;
    dump->payload_header.Magic = DUMP_PAYLOAD_MAGIC;
    dump->payload_header.Root.ChunkDescriptor.Magic = DUMP_CHUNK_MAGIC_FINALIZED;
    dump->payload_header.Root.ChunkDescriptor.Type = DumpChunkType_Aggregate;
    dump->payload_header.Root.ChunkDescriptor.HeaderSize = sizeof(DUMP_AGGREGATE_DESCRIPTOR);
    dump->payload_header.Root.ChunkDescriptor.PayloadSize = sizeof(DUMP_CHUNK_DESCRIPTOR) + payload_size;
    dump->payload_header.Root.ChunkCount = 1;

    dump->first_chunk_desc.Magic = DUMP_CHUNK_MAGIC_FINALIZED;
    dump->first_chunk_desc.HeaderSize = sizeof(DUMP_CHUNK_DESCRIPTOR);
    dump->first_chunk_desc.PayloadSize = payload_size;
    for (uint32_t i = 0; i < payload_size; i++)
    {
        dump->payload[i] = payload;
    }
}

//
// Tests
//
static int test_setup(void** ctx)
{
    FPFW_UNUSED(ctx);

    build_crash_dump_data(DIE_0, CRASH_DUMP_CORE_MCP, 40, 0xAA);
    build_crash_dump_data(DIE_0, CRASH_DUMP_CORE_SCP, 80, 0xBB);

    build_crash_dump_data(DIE_1, CRASH_DUMP_CORE_HSP, 120, 0xCC);

    return 0;
}

static int test_teardown(void** ctx)
{
    FPFW_UNUSED(ctx);
    memset(crash_dump_region, 0, sizeof(crash_dump_region));

    return 0;
}

TEST_FUNCTION(test_crash_dump_pldm_transfer_dump_no_dump, nullptr, test_teardown)
{
    // Set expectations
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    // Act
    crash_dump_pldm_transfer_dump();

    // Assert no transfer occurred
    assert_true(crash_dump_pldm_transfer_completed());
}

TEST_FUNCTION(test_crash_dump_pldm_transfer_dump_error, test_setup, test_teardown)
{
    // Set expectations
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_FAIL);
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);

    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    // Act
    crash_dump_pldm_transfer_dump();

    // Assert no transfer occurred
    assert_true(crash_dump_pldm_transfer_completed());
}

TEST_FUNCTION(test_crash_dump_pldm_transfer_dump, test_setup, test_teardown)
{
    crash_dump_header_t full_header = {.status = CRASH_DUMP_IN_USE};
    crash_dump_type_context_t type_context = {.type = CRASH_DUMP_TYPE_FULL, .header = &full_header};
    crash_dump_context_t context = {.type_ctx = {NULL, &type_context}, .core_index = CRASH_DUMP_CORE_MCP};
    will_return_always(__wrap_crash_dump_context, &context);

    // Set expectations
    will_return(__wrap_atu_map, SILIBS_SUCCESS);

    will_return(__wrap_fpfw_pldm_service_raise_platform_event, FPFW_STATUS_SUCCESS);
    expect_function_call(__wrap_fpfw_pldm_service_raise_platform_event);

    expect_function_call(__wrap_gpio_set_output);
    expect_value(__wrap_gpio_set_output, gpio_pin_id, GPIO_CTRL_PIN_ID(MSCP_EXP_GPIO_6, 3));
    expect_value(__wrap_gpio_set_output, level, true);

    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Act
    crash_dump_pldm_transfer_dump();

    // Assert no transfer occurred
    assert_false(crash_dump_pldm_transfer_completed());
}

TEST_FUNCTION(test_crash_dump_transfer_dump_platform_event_cb, test_setup, test_teardown)
{
    crash_dump_stream_t crash_dump_stream = {};
    uint8_t dest[1024] = {0}; // Destination buffer for the transfer

    // Open the stream
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    crash_dump_stream_open(&crash_dump_stream);

    uint64_t expected_header_size = sizeof(DUMP_HEADER) + sizeof(DUMP_METADATA_HEADER) +
                                    sizeof(DUMP_PAYLOAD_HEADER) + 3 * sizeof(DUMP_CHUNK_DESCRIPTOR);
    assert_true(crash_dump_stream.header_aggregate.FileSize ==
                240 + expected_header_size); // 240 = 80 + 120 + 40 (MCP) + 80 (SCP) + 120 (HSP) payload sizes in test data

    // Get the aggregated crash dump data
    crash_dump_transfer_dump_platform_event_cb((void*)&crash_dump_stream,
                                               dest,
                                               0,
                                               240 + sizeof(DUMP_HEADER) + sizeof(DUMP_METADATA_HEADER) +
                                                   sizeof(DUMP_PAYLOAD_HEADER) + 3 * sizeof(DUMP_CHUNK_DESCRIPTOR));

    // Verify merged crash dump data
    crash_dump_file_test_t* merged_dump = (crash_dump_file_test_t*)dest;
    crash_dump_file_test_t* die0_mcp_dump = &crash_dump_region[0 * CRASH_DUMP_CORE_NUM + CRASH_DUMP_CORE_MCP];
    crash_dump_file_test_t* die0_scp_dump = &crash_dump_region[0 * CRASH_DUMP_CORE_NUM + CRASH_DUMP_CORE_SCP];
    crash_dump_file_test_t* die1_hsp_dump = &crash_dump_region[1 * CRASH_DUMP_CORE_NUM + CRASH_DUMP_CORE_HSP];

    // Check the headers
    assert_memory_equal(&merged_dump->header, &crash_dump_stream.header_aggregate, sizeof(DUMP_HEADER));
    assert_memory_equal(&merged_dump->metadata_header, &crash_dump_stream.metadata_header_aggregate, sizeof(DUMP_METADATA_HEADER));
    assert_memory_equal(&merged_dump->payload_header, &crash_dump_stream.payload_header_aggregate, sizeof(DUMP_PAYLOAD_HEADER));

    // Check the MCP0, SCP0, and HSP1 chunks
    uint32_t test_offset = 0;
    uint32_t test_size = die0_mcp_dump->first_chunk_desc.HeaderSize + die0_mcp_dump->first_chunk_desc.PayloadSize;
    assert_memory_equal(&merged_dump->first_chunk_desc, &die0_mcp_dump->first_chunk_desc, test_size);

    test_offset += test_size;
    test_size = die0_scp_dump->first_chunk_desc.HeaderSize + die0_scp_dump->first_chunk_desc.PayloadSize;
    assert_memory_equal(((uint8_t*)&merged_dump->first_chunk_desc) + test_offset, &die0_scp_dump->first_chunk_desc, test_size);

    test_offset += test_size;
    test_size = die1_hsp_dump->first_chunk_desc.HeaderSize + die1_hsp_dump->first_chunk_desc.PayloadSize;
    assert_memory_equal(((uint8_t*)&merged_dump->first_chunk_desc) + test_offset, &die1_hsp_dump->first_chunk_desc, test_size);

    // Reset test
    crash_dump_stream_reset(&crash_dump_stream);
    assert_true(crash_dump_stream.current_chunk_index == 0);
    assert_true(crash_dump_stream.current_chunk_offset == 0);

    // Crash dump stream read failure test
    uint32_t read_size = crash_dump_stream_read(NULL, dest, 0);
    assert_true(read_size == 0); // Expect no data read

    // Close the stream
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);
    crash_dump_stream_close(&crash_dump_stream, true);
}

TEST_FUNCTION(test_crash_dump_pldm_on_ppe_complete, test_setup, test_teardown)
{
    crash_dump_header_t full_header = {.status = CRASH_DUMP_IN_TRANSFER};
    crash_dump_type_context_t type_context = {.type = CRASH_DUMP_TYPE_FULL, .header = &full_header};
    crash_dump_context_t context = {.type_ctx = {NULL, &type_context}, .core_index = CRASH_DUMP_CORE_MCP};
    will_return_always(__wrap_crash_dump_context, &context);

    // Open the crash dump stream
    crash_dump_stream_t crash_dump_stream = {};
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    crash_dump_stream_open(&crash_dump_stream);

    // Set expectations
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Act
    crash_dump_pldm_on_ppe_complete(FPFW_PLDM_CC_SUCCESS, &crash_dump_stream);

    // Assert
    assert_true(full_header.status == CRASH_DUMP_NOT_IN_USE);
}

TEST_FUNCTION(test_crash_dump_pldm_on_ppe_complete_with_fail, test_setup, test_teardown)
{
    crash_dump_header_t full_header = {.status = CRASH_DUMP_IN_TRANSFER};
    crash_dump_type_context_t type_context = {.type = CRASH_DUMP_TYPE_FULL, .header = &full_header};
    crash_dump_context_t context = {.type_ctx = {NULL, &type_context}, .core_index = CRASH_DUMP_CORE_MCP};
    will_return_always(__wrap_crash_dump_context, &context);

    // Open the crash dump stream
    crash_dump_stream_t crash_dump_stream = {};
    will_return(__wrap_atu_map, SILIBS_SUCCESS);
    crash_dump_stream_open(&crash_dump_stream);

    // Set expectations
    will_return(__wrap_atu_unmap, SILIBS_SUCCESS);

    expect_any(__wrap_wait_for_semaphore, id);
    expect_any(__wrap_wait_for_semaphore, key);
    expect_function_call(__wrap_wait_for_semaphore);
    expect_any(__wrap_release_semaphore, id);
    expect_function_call(__wrap_release_semaphore);

    // Act
    crash_dump_pldm_on_ppe_complete(FPFW_PLDM_CC_ERROR, &crash_dump_stream);

    // Assert
    assert_true(full_header.status == CRASH_DUMP_IN_USE);
}

TEST_FUNCTION(test_get_crash_dump_region_address, nullptr, nullptr)
{
    atu_map_entry_t die1_map_entry = {.mscp_start_address = 0x1000000};
    uint8_t* address = __real_get_crash_dump_region_address(&die1_map_entry, DIE_1, CRASH_DUMP_CORE_SCP);

    // Check if the address is correctly calculated
    assert_non_null(address);
    assert_true((uintptr_t)address == (uintptr_t)(0x1000000 + (CRASH_DUMP_CORE_SCP * CRASH_DUMP_FULL_SIZE_PER_CORE)));

    address = __real_get_crash_dump_region_address(&die1_map_entry, DIE_0, CRASH_DUMP_CORE_SCP);
    assert_non_null(address);
    assert_true((uintptr_t)address == (uintptr_t)(CRASH_DUMP_FULL_SCP_ADDR));

    address = __real_get_crash_dump_region_address(NULL, DIE_0, CRASH_DUMP_CORE_MCP);
    assert_null(address); // Expect null when die1_map_entry is NULL
}
}