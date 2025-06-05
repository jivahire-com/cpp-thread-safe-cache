# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies Message Transfer System (MTS) Data Collection Protocol (DCP) functionality.
...
...    This robot test verifies [DCP] CLIENT_GET_PLATFORM_INFORMATION functionality by:
...    1. Testing the DCP commands
...    2. Verifying command responses
...    3. Validating proper setup and teardown of test environment

Library    ${CURDIR}/../../library

Library    library.mts_tests.mts_dcp_test.MtsDcpTest
...    workspace_config=${WORKSPACE_CONFIG}
...    default_log_home=${LOG_DIR}
...    fw_payload_path=${PAYLOAD_DIR}
...    host_config=${HOST_CONFIG_DIR}/${HOST_JSON}
...    WITH NAME    mts_dcp_test_lib

Suite Setup       Setup Test Environment
Suite Teardown    Teardown Test Environment

*** Keywords ***
Setup Test Environment
    ${test_lib}=    Get Library Instance    mts_dcp_test_lib
    ${setup_result}=    Call Method    ${test_lib}    setup
    Should Be True    ${setup_result}    Failed to setup DUT

Teardown Test Environment
    ${test_lib}=    Get Library Instance    mts_dcp_test_lib
    Run Keyword And Ignore Error    Call Method    ${test_lib}    teardown

*** Test Cases ***
Test Case - [DCP] CLIENT_GET_PLATFORM_INFORMATION Test
    [Documentation]    Verifies MTS CLIENT_GET_PLATFORM_INFORMATION command functionality.
    ...
    ...    Test Steps:

    [Tags]    ssi    ssi_mts_event_get_platform_info    mts    mts_client    TEST_CASE_ID:2572935

    ${test_lib} =    Get Library Instance    mts_dcp_test_lib
    Log To Console    \nExecuting client_get_platform_information Test...
    ${test_result} =    Call Method    ${test_lib}    test_client_get_platform_information
    Should be True    ${test_result}
    