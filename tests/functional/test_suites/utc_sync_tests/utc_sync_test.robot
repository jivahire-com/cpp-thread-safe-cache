# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space.
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Functional Tests for UTC Management firmware.
...              Validates UTC time synchronization between BMC and MCP
...              using the 'utc time' MCP CLI command.
Suite Setup      Setup Test Environment
Suite Teardown   Teardown Test Environment


# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.utc_sync_tests.utc_sync_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         dut_platform=KingsgateSVP
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    utc_sync_lib

*** Variables ***
${Success}    ${True}
${Failure}    ${False}

*** Keywords ***
Setup Test Environment
    ${test_lib} =       Get Library Instance                 utc_sync_lib
    ${setup_result} =   Call Method    ${test_lib}    setup_dut
    Should Be True      ${setup_result}    Failed to setup DUT

Teardown Test Environment
    ${test_lib} =       Get Library Instance                 utc_sync_lib
    ${teardown_result} =    Call Method    ${test_lib}    teardown_dut
    Should Be True      ${teardown_result}    Failed to teardown DUT

*** Test Cases ***
Test Case - 1. Capture UTC Times From BMC And MCP
    [Documentation]    Confirms PLDM service is active, then captures MCP 'utc time'
    ...                and BMC 'timedatectl' output twice with a 40-second delay.
    ...                Validates that UTC times match within tolerance on both samples.
    ${result}=    utc_sync_lib.testcase1_capture_utc_times
    Should Be True    ${result}[pldm_active]    PLDM service is not active
    Should Be True    ${result}[time_match_1]    First sample: UTC times do not match
    Should Be True    ${result}[time_match_2]    Second sample: UTC times do not match
    Should Be True    ${result}[success]    UTC time synchronization test failed
