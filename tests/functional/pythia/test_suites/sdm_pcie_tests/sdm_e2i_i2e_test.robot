# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies the sdm e2i_i2e command from AP core 0.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.sdm_tests.sdm_e2i_i2e_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    sdm_e2i_i2e_test_lib

*** Variables ***
@{PASS_RESPONSE}    User Defined Loopback Test End!
@{FAIL_RESPONSE}    FAIL

*** Test Cases ***
Test Case - AP: SDM E2I I2E Test: FR [1738499]
    [Documentation]     Test the SDM E2I I2E of SDM from AP core 0.

    # Get an instance of the fpga_sdm_memcpy_tests_lib library
    ${test_lib} =       Get Library Instance      sdm_e2i_i2e_test_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         sdm_e2i_i2e_test       ${PASS_RESPONSE}        ${FAIL_RESPONSE}

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}