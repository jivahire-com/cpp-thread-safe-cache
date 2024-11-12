//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file warm_start_test.cpp
 * Test for Warm Start
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // IWYU pragma: keep
#include <cstdint>         // IWYU pragma: keep
#include <tx_api.h>

extern "C" {
#include <../src/warm_start_i.h>
#include <FpFwUtils.h> // for FPFW_UNUSED
#include <warm_start.h>

/*-- Symbolic Constant Macros (defines) --*/

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/

/*-- Declarations (Statics and globals) --*/

extern ws_data_list_t* p_ws_list;
extern uint32_t ws_size;
ws_data_list_t* temp_p_ws_list;
extern int32_t calculate_checksum(char* p_data, uint32_t length);
extern void* ws_data_get(mod_ws_data_id_t id, uint32_t* p_size);
extern void init_ws_mutex();
/*----------- Mock Functions -------------*/

/*------------- Functions ----------------*/

static int setup(void** pContext)
{
    FPFW_UNUSED(pContext);
    temp_p_ws_list = &p_ws_list[0];
    init_ws_mutex();

    expect_function_call(__wrap__txe_mutex_create);
    expect_string(__wrap__txe_mutex_create, name_ptr, "ws data mutex");

    warm_start_init();

    return 0;
}

static int teardown(void** pContext)
{
    FPFW_UNUSED(pContext);
    p_ws_list = &temp_p_ws_list[0];
    return 0;
}

TEST_FUNCTION(calculate_checksum_test, NULL, NULL)
{
    assert_int_equal(calculate_checksum(0, 0), 0);

    const char check_array[] = {'c', 'h', 'e', 'k', 's', 'u', 'm'};
    const int32_t checksum_value = -752;

    assert_int_equal(calculate_checksum((char*)&check_array, (uint32_t)sizeof(check_array)), checksum_value);
}

TEST_FUNCTION(ws_init_function, NULL, NULL)
{
    init_ws_mutex();

    expect_function_call(__wrap__txe_mutex_create);
    expect_string(__wrap__txe_mutex_create, name_ptr, "ws data mutex");

    warm_start_init();

    assert_int_equal(p_ws_list, SCP_WARM_START_BASE);
    assert_int_equal(ws_size, SCP_WARM_START_SIZE);
}

TEST_FUNCTION(ws_data_get_null_psize, setup, teardown)
{

    ws_data_list_t test_p_ws_list = {.magic_id = 1,
                                     .entry = {.id = WARM_START_ID_RESERVED, .checksum = 0, .size = 0xF, .data = 0xFF}};
    p_ws_list = &test_p_ws_list;

    assert_non_null(&test_p_ws_list);
    assert_null(ws_data_get(WARM_START_ID_RESERVED, NULL));
}

TEST_FUNCTION(ws_data_get_magicid_test, setup, teardown)
{
    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);
    ws_data_list_t test_p_ws_list = {.magic_id = WARM_START_MAGIC_ID,
                                     .entry = {.id = WARM_START_ID_RESERVED, .checksum = -255, .size = 0x1, .data = 255}};
    p_ws_list = &test_p_ws_list;

    uint32_t size = 0xFF;

    assert_non_null(&test_p_ws_list);
    assert_non_null(ws_data_get(WARM_START_ID_RESERVED, &size));
    assert_int_equal(size, 0x1);
}

TEST_FUNCTION(ws_data_put_update_size, setup, teardown)
{
    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);

    ws_data_entry_t test_p_next = {.id = WARM_START_ID_LAST, .checksum = -255, .size = 0x1, .data = 0xFF};

    ws_data_list_t test_p_ws_list = {.magic_id = 0, .entry = {.p_next = &test_p_next}};

    p_ws_list = &test_p_ws_list;

    uint32_t size = 0x02;

    uint8_t data[] = {6, 1};
    void* p_data = &data;

    assert_non_null(ws_data_put(WARM_START_ID_LAST, p_data, size));
}

TEST_FUNCTION(ws_data_put_expect_id_last, setup, teardown)
{
    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);

    ws_data_entry_t test_p_next = {.id = WARM_START_ID_LAST, .checksum = -255, .size = 0x1, .data = 0xFF};
    ws_data_list_t test_p_ws_list = {
        .magic_id = WARM_START_MAGIC_ID,
        .entry = {.p_next = &test_p_next, .id = WARM_START_ID_RESERVED, .checksum = -255, .size = 0x1, .data = 0xFF}};
    p_ws_list = &test_p_ws_list;
    uint32_t size = 0x1;

    uint8_t data = 6;
    void* p_data = &data;

    assert_non_null(ws_data_put(WARM_START_ID_RESERVED, p_data, size));

    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);

    assert_ptr_equal(ws_data_put(WARM_START_ID_LAST, p_data, size), &(test_p_next.data));
}

TEST_FUNCTION(ws_data_put_expect_id_reserved, setup, teardown)
{
    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);

    ws_data_entry_t test_p_next = {.id = WARM_START_ID_RESERVED, .checksum = -255, .size = 0x1, .data = 0xFF};
    ws_data_list_t test_p_ws_list = {
        .magic_id = WARM_START_MAGIC_ID,
        .entry = {.p_next = &test_p_next, .id = WARM_START_ID_LAST, .checksum = -255, .size = 0x1, .data = 0xFF}};
    p_ws_list = &test_p_ws_list;
    uint32_t size = 0x1;

    uint8_t data = 6;
    void* p_data = &data;

    assert_ptr_equal(ws_data_put(WARM_START_ID_RESERVED, p_data, size), &(test_p_next.data));
}

TEST_FUNCTION(ws_data_put_current_expected_id_reserved, setup, teardown)
{
    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);

    ws_data_entry_t test_p_next = {.id = WARM_START_ID_RESERVED, .checksum = -255, .size = 0x1, .data = 0xFF};
    ws_data_list_t test_p_ws_list = {
        .magic_id = WARM_START_MAGIC_ID,
        .entry = {.p_next = &test_p_next, .id = WARM_START_ID_RESERVED, .checksum = -255, .size = 0x1, .data = 0xFF}};
    p_ws_list = &test_p_ws_list;
    uint32_t size = 0x1;

    uint8_t data = 6;
    void* p_data = &data;

    assert_ptr_equal(ws_data_put(WARM_START_ID_RESERVED, p_data, size), &(test_p_ws_list.entry.data));
}

TEST_FUNCTION(ws_data_put_current_expected_id_last, setup, teardown)
{
    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);

    ws_data_entry_t test_p_next = {.id = WARM_START_ID_LAST, .checksum = -255, .size = 0x1, .data = 0xFF};
    ws_data_list_t test_p_ws_list = {
        .magic_id = WARM_START_MAGIC_ID,
        .entry = {.p_next = &test_p_next, .id = WARM_START_ID_LAST, .checksum = -255, .size = 0x1, .data = 0xFF}};
    p_ws_list = &test_p_ws_list;
    uint32_t size = 0x1;

    uint8_t data = 6;
    void* p_data = &data;

    assert_ptr_equal(ws_data_put(WARM_START_ID_LAST, p_data, size), &(test_p_ws_list.entry.data));
}

TEST_FUNCTION(ws_data_put_current_expected_id_reserved_cli_test, setup, teardown)
{
    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);

    ws_data_entry_t test_p_next = {.id = WARM_START_ID_RESERVED_CLI_TEST, .checksum = -255, .size = 0x1, .data = 0xFF};
    ws_data_list_t test_p_ws_list = {
        .magic_id = WARM_START_MAGIC_ID,
        .entry = {.p_next = &test_p_next, .id = WARM_START_ID_RESERVED, .checksum = -255, .size = 0x1, .data = 0xFF}};
    p_ws_list = &test_p_ws_list;
    uint32_t size = 0x1;

    uint8_t data = 6;
    void* p_data = &data;

    assert_ptr_equal(ws_data_put(WARM_START_ID_RESERVED_CLI_TEST, p_data, size), &(test_p_next.data));
}

TEST_FUNCTION(ws_data_expected_equal_value_for_cli_id, setup, teardown)
{
    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);

    ws_data_entry_t test_p_next = {.id = WARM_START_ID_LAST, .checksum = -255, .size = 0x1, .data = 0xFF};
    ws_data_list_t test_p_ws_list = {.magic_id = 0, .entry = {.p_next = &test_p_next}};

    p_ws_list = &test_p_ws_list;
    uint32_t size = 0x1;

    uint8_t data = 6;
    void* p_data = &data;

    assert_non_null(ws_data_put(WARM_START_ID_RESERVED_CLI_TEST, p_data, size));

    expect_function_call(__wrap__txe_mutex_get);
    expect_function_call(__wrap__txe_mutex_put);

    assert_int_equal(*(int*)ws_data_get(WARM_START_ID_RESERVED_CLI_TEST, &size), data);
}
}
