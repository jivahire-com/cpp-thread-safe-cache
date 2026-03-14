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
Test Case - 1. Parallel Capture UTC Times From BMC And MCP
    [Documentation]    Captures MCP 'utc time' and BMC 'timedatectl' in true parallel
    ...                (MCP over UART, BMC over SSH) ten times with 40-second delays.
    ...                Parallel capture eliminates sequential command latency from the
    ...                measured difference, giving a direct measurement of clock drift.
    ...                Includes debug instrumentation: per-command latency and
    ...                raw vs corrected diff.
    ...                Uses 4-second tolerance.
    ${result}=    utc_sync_lib.testcase2_parallel_capture_utc_times
    Should Be True    ${result}[pldm_active]    PLDM service is not active
    Should Be True    ${result}[time_match_1]    Sample 1: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[time_match_2]    Sample 2: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[time_match_3]    Sample 3: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[time_match_4]    Sample 4: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[time_match_5]    Sample 5: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[time_match_6]    Sample 6: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[time_match_7]    Sample 7: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[time_match_8]    Sample 8: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[time_match_9]    Sample 9: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[time_match_10]    Sample 10: UTC times do not match within ${result}[tolerance_sec]s tolerance
    Should Be True    ${result}[success]    UTC time synchronization failed - not all parallel samples synchronized
