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
Test Case - 1. Verify UTC Time Sync Between BMC And MCP
    [Documentation]    Verifies BMC NTP is synchronized, executes 'utc time' on MCP,
    ...                validates the timestamp is a real epoch value (not uptime ticks),
    ...                and confirms BMC-MCP drift is within 10 seconds.
    ${result}=  utc_sync_lib.testcase1_verify_utc_sync
    Should Be Equal   ${result}   ${Success}
