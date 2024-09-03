# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies cded compress decompress completion from AP core 0.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.sdm_tests.cded_compress_decompress_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    cded_compress_decompress_test_lib

*** Variables ***
@{PASS_RESPONSE}        compare_data_buffer: INFO: SUCCESS
...                     wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0

@{FAIL_RESPONSE}        FAIL

*** Test Cases ***
Test Case - AP: cded compress decompress test Test: FR [1423800]
    [Documentation]     Test the CDED memcpy on CDED from AP core 0.

    # Get an instance of the fpga_cded_memcpy_tests_lib library
    ${test_lib} =       Get Library Instance      cded_compress_decompress_test_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         cded_compress_decompress_test       ${PASS_RESPONSE}        ${FAIL_RESPONSE}

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}