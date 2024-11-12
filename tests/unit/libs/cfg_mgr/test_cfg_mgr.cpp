//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

/**
 * @file test_cfg_mgr.cpp
 * Config Manager unit tests
 */

/*------------- Includes -----------------*/
#include <CMockaWrapper.h> // for check_expected_ptr, expect_fun...
extern "C" {
#include <fpfw_init.h>
}
#include <cfg_mgr.h>
#include <cstddef> // for NULL, size_t
#include <cstdint> // for uint8_t, uint32_t
#include <cstdio>  // for printf
#include <cstring>
#include <fpfw_cfg_mgr.h>
#include <fpfw_cfg_mgr_init.h>
#include <fpfw_status.h>
#include <map>
#include <utility> // for pair
#include <vector>
#include <xstring> // for string, operator<
#include <xtree>   // for _Tree_iterator, _Tree<>::iterator

/*-- Symbolic Constant Macros (defines) --*/
#define PROFILE_COUNT 2
#define DATA_A                       \
    {                                \
        0x11, 0x22, 0x33, 0x44, 0x55 \
    }
#define DATA_B                       \
    {                                \
        0xFF, 0xEE, 0xDD, 0xCC, 0xBB \
    }
#define COUNTER 0x12345678

const double TOLERANCE = 0.0001;
const int INTEGER_KNOB_DB = 42;
const float FLOAT_KNOB_DB_E = 3.141;
const int INTEGER_KNOB_DEF = 100;
const float FLOAT_KNOB_DEF = 1.414;
const int COMPLEX_KNOB3B_DEF = 2;
// const int COMPLEX_KNOB3B_A = 4;

/*------------- Typedefs -----------------*/

/*-------- Function Prototypes -----------*/
extern void apply_default();
extern void apply_db(fpfw_cfg_mgr_db_t* db);
extern void apply_profile(profile_t* override);

/*-- Declarations (Statics and globals) --*/
static std::map<std::string, std::vector<uint8_t>> knob_store;

/*------------- Functions ----------------*/

//
// Mocks
//
static bool mock_read_knob_fn(const fpfw_cfg_mgr_guid_t* knob_namespace, const char* knob_name, uint8_t* data, size_t data_size, void* ctx)
{
    assert_non_null(knob_namespace);
    assert_non_null(ctx);
    std::string key = std::string(knob_name);
    auto it = knob_store.find(key);
    if (it != knob_store.end() && it->second.size() == data_size)
    {
        std::memcpy(data, it->second.data(), data_size);
        return true;
    }
    return false;
}

static bool mock_write_knob_fn(const fpfw_cfg_mgr_guid_t* knob_namespace,
                               const char* knob_name,
                               const uint8_t* data,
                               size_t data_size,
                               void* ctx)
{
    assert_non_null(knob_namespace);
    assert_non_null(ctx);
    std::string key = std::string(knob_name);
    knob_store[key] = std::vector<uint8_t>(data, data + data_size);
    return true;
}

static void mock_db_setup()
{
    fpfw_cfg_mgr_guid_t mock_guid = {0};
    void* mock_ctx = (void*)1;
    mock_write_knob_fn(&mock_guid, "INTEGER_KNOB", (uint8_t*)&INTEGER_KNOB_DB, sizeof(int), &mock_ctx);
    mock_write_knob_fn(&mock_guid, "FLOAT_KNOB", (uint8_t*)&FLOAT_KNOB_DB_E, sizeof(float), &mock_ctx);
}

//
// Setup/Teardown
//
static int clean(void** state)
{
    (void)state;
    knob_store.clear();
    apply_default();
    return 0;
}

static int init_setup_m(void** state)
{
    (void)state;
    mock_db_setup();

    fpfw_cfg_mgr_config_t cfg = {.mission_mode = true,
                                 .profile_id = 0,
                                 .write_knob_fn = mock_write_knob_fn,
                                 .read_knob_fn = mock_read_knob_fn,
                                 .db_ctx = (void*)1};
    fpfw_status_t status = fpfw_cfg_mgr_init(&cfg);
    printf("setup_m::status-> %d\n", status);
    return 0;
}

static int init_setup_a(void** state)
{
    (void)state;
    mock_db_setup();
    fpfw_cfg_mgr_config_t cfg = {.mission_mode = false,
                                 .profile_id = 0,
                                 .write_knob_fn = mock_write_knob_fn,
                                 .read_knob_fn = mock_read_knob_fn,
                                 .db_ctx = (void*)1};
    fpfw_status_t status = fpfw_cfg_mgr_init(&cfg);
    printf("setup_a::status-> %d\n", status);
    return 0;
}

static int init_setup_b(void** state)
{
    (void)state;
    mock_db_setup();

    fpfw_cfg_mgr_config_t cfg = {.mission_mode = false,
                                 .profile_id = 1,
                                 .write_knob_fn = mock_write_knob_fn,
                                 .read_knob_fn = mock_read_knob_fn,
                                 .db_ctx = (void*)1};
    fpfw_status_t status = fpfw_cfg_mgr_init(&cfg);
    printf("setup_b::status-> %d\n", status);
    return 0;
}

//
// Tests
//
//
// config codegen tests
//
// These tests are intended to verify the codegen used to generate the accessor functions
// for configution variables.
//
// These tests are NOT verifying the parsing and handling of the XML. We assume that the XML
// and VariableList.py are properly parsing the schema. These tests focus on the transformation
// to C accessors.
//
// These tests serve two functions:
//   1. Just by compiling and linking, these limits exercise the code generation features and
//      may uncover errors that appear as compiler errors or warnings.
//   2. By checking the limits and returned defaults of function the tests verify the mapping
//      between the XML and the outcome of generated C functions
//
// The code generation can produce fairly complex types with unenumerable permutations. Because
// the implementation of the code generation will re-use generation and verification routines
// the same way regardless of whether it is in the root of a knob or embedded within a struct
// or array, we only verify the limits of primatives, and then confirm that complex knobs with
// any internal limits behave correctly. We do not attempt to verify all possible nexted combinations.
//

TEST_FUNCTION(cmtests_basic_defaults, NULL, NULL)
{
    // For each basic type
    //   * Verify knobs without a "default" specified return 0
    //   * Verify the sizeof the type matches the expected values
    //      C cannot verify the "typeof", but this ensures that small knobs like uint8_t aren't
    //      unexpectedly being implemented with larger C types
    //   * Verify knobs with a "default" specified return that value
    //   * Verify defaults values that are in the highest range for that type
    //      i.e. negative values can be returned for signed types
    //           large values can be returned for large types
    //
    // These tests are to ensure that the range of values declared in the XML will

    // uint8_t
    assert_int_equal(0, config_get_k_uint8_t());
    assert_true(sizeof(config_get_k_uint8_t()) == sizeof(uint8_t));
    assert_int_equal(10, config_get_k_uint8_t_d10());

    // int8_t
    assert_int_equal(0, config_get_k_int8_t());
    assert_true(sizeof(config_get_k_int8_t()) == sizeof(int8_t));
    assert_int_equal(10, config_get_k_int8_t_d10());
    assert_int_equal(-10, config_get_k_int8_t_dn10());

    // uint16_t
    assert_int_equal(0, config_get_k_uint16_t());
    assert_true(sizeof(config_get_k_uint16_t()) == sizeof(uint16_t));
    assert_int_equal(1000, config_get_k_uint16_t_d1000());

    // int16_t
    assert_int_equal(0, config_get_k_int16_t());
    assert_true(sizeof(config_get_k_int16_t()) == sizeof(int16_t));
    assert_int_equal(1000, config_get_k_int16_t_d1000());
    assert_int_equal(-1000, config_get_k_int16_t_dn1000());

    // uint32_t
    assert_int_equal(0, config_get_k_uint32_t());
    assert_true(sizeof(config_get_k_uint32_t()) == sizeof(uint32_t));
    assert_int_equal(1000000, config_get_k_uint32_t_d1000000());

    // int32_t
    assert_int_equal(0, config_get_k_int32_t());
    assert_true(sizeof(config_get_k_int32_t()) == sizeof(int32_t));
    assert_int_equal(1000000, config_get_k_int32_t_d1000000());
    assert_int_equal(-1000000, config_get_k_int32_t_dn1000000());

    // uint64_t
    assert_int_equal(0, config_get_k_uint64_t());
    assert_true(sizeof(config_get_k_uint64_t()) == sizeof(uint64_t));
    assert_int_equal(10000000000, config_get_k_uint64_t_d10000000000());

    // int64_t
    assert_int_equal(0, config_get_k_int64_t());
    assert_true(sizeof(config_get_k_int64_t()) == sizeof(int64_t));
    assert_int_equal(10000000000, config_get_k_int64_t_d10000000000());
    assert_int_equal(-10000000000, config_get_k_int64_t_dn10000000000());

    // float
    assert_true(0.0 == config_get_k_float());
    assert_true(sizeof(config_get_k_float()) == sizeof(float));
    assert_true(10.5 == config_get_k_float_d10_5());

    // double
    assert_true(0.0 == config_get_k_double());
    assert_true(sizeof(config_get_k_double()) == sizeof(double));
    assert_true(10.000005 == config_get_k_double_d10_000005());
    assert_true(-10.000005 == config_get_k_double_dn10_000005());

    // bool
    assert_true(false == config_get_k_bool());
    assert_true(sizeof(config_get_k_bool()) == sizeof(bool));
    assert_true(true == config_get_k_bool_dtrue());
}

TEST_FUNCTION(runtime_init_check_success, NULL, NULL)
{
    // Verify that the configuration manager can be initialized with a valid configuration
    // and that the configuration manager can be initialized multiple times
    fpfw_cfg_mgr_config_t cfg = {.mission_mode = false,
                                 .profile_id = 0,
                                 .write_knob_fn = mock_write_knob_fn,
                                 .read_knob_fn = mock_read_knob_fn,
                                 .db_ctx = (void*)1};
    fpfw_status_t status = fpfw_cfg_mgr_init(&cfg);
    assert_int_equal(FPFW_STATUS_SUCCESS, status);

    status = fpfw_cfg_mgr_init(&cfg);
    assert_int_equal(FPFW_STATUS_SUCCESS, status);
}

TEST_FUNCTION(test_init_invalid_profile_id, NULL, NULL)
{
    fpfw_cfg_mgr_config_t cfg = {.mission_mode = false,
                                 .profile_id = PROFILE_COUNT, // Invalid profile ID
                                 .write_knob_fn = mock_write_knob_fn,
                                 .read_knob_fn = mock_read_knob_fn,
                                 .db_ctx = (void*)1};
    fpfw_status_t status = fpfw_cfg_mgr_init(&cfg);
    assert_int_equal(FPFW_STATUS_INVALID_ARGS, status);
}

TEST_FUNCTION(test_init_invalid_db, NULL, NULL)
{
    fpfw_cfg_mgr_config_t cfg = {.mission_mode = false,
                                 .profile_id = 0,
                                 .write_knob_fn = mock_write_knob_fn,
                                 .read_knob_fn = nullptr,
                                 .db_ctx = (void*)1};
    fpfw_status_t status = fpfw_cfg_mgr_init(&cfg);
    assert_int_equal(FPFW_STATUS_INVALID_ARGS, status);
}

TEST_FUNCTION(test_init_invalid_db_ctx, NULL, NULL)
{
    fpfw_status_t status = fpfw_cfg_mgr_init(nullptr);
    assert_int_equal(FPFW_STATUS_NULL_POINTER, status);
}

TEST_FUNCTION(test_init_success, NULL, NULL)
{
    fpfw_cfg_mgr_config_t cfg = {.mission_mode = false,
                                 .profile_id = 0,
                                 .write_knob_fn = mock_write_knob_fn,
                                 .read_knob_fn = mock_read_knob_fn,
                                 .db_ctx = (void*)1};
    fpfw_status_t status = fpfw_cfg_mgr_init(&cfg);
    assert_int_equal(FPFW_STATUS_SUCCESS, status);
}

TEST_FUNCTION(test_knob_get_b, init_setup_b, clean)
{
    assert_true(config_get_BOOLEAN_KNOB());
    assert_true(config_get_BOOLEAN_KNOB2());

    child_t test_3b = config_get_COMPLEX_KNOB3b();

    assert_int_equal(COMPLEX_KNOB3B_DEF, test_3b.data[0]);
    assert_int_equal(SECOND, test_3b.mode);
}

TEST_FUNCTION(test_knob_get_db_m, init_setup_m, clean)
{
    // mission mode; no db override
    assert_int_equal(INTEGER_KNOB_DEF, config_get_INTEGER_KNOB());
    assert_float_equal(FLOAT_KNOB_DEF, config_get_FLOAT_KNOB(), TOLERANCE);
}

TEST_FUNCTION(test_knob_get_db, init_setup_a, clean)
{
    assert_int_equal(INTEGER_KNOB_DB, config_get_INTEGER_KNOB());
    assert_float_equal(FLOAT_KNOB_DB_E, config_get_FLOAT_KNOB(), TOLERANCE);
}
