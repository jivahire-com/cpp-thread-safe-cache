# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies Data Collection System (DCS) Data Collection Protocol (DCP) functionality.
...
...    This robot test verifies the DCS library functionality by:
...    1. Testing the DCP commands
...    2. Verifying command responses
...    3. Validating proper setup and teardown of test environment

Library    ${CURDIR}/../../pylibs

Library    pylibs.dcs_tests.dcs_dcp_test.DcsDcpTest
...    workspace_config=${WORKSPACE_CONFIG}
...    default_log_home=${LOG_DIR}
...    fw_payload_path=${PAYLOAD_DIR}
...    host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw.json
...    WITH NAME    dcs_dcp_test_lib

Suite Setup       Setup Test Environment
Suite Teardown    Teardown Test Environment

*** Keywords ***
Setup Test Environment
    ${test_lib}=    Get Library Instance    dcs_dcp_test_lib
    ${setup_result}=    Call Method    ${test_lib}    setup
    Should Be True    ${setup_result}    Failed to setup DUT

Teardown Test Environment
    ${test_lib}=    Get Library Instance    dcs_dcp_test_lib
    Run Keyword And Ignore Error    Call Method    ${test_lib}    teardown

*** Test Cases ***
Test Case - DCS Client Start Stop Test
    [Documentation]    Verifies DCS client start and stop command functionality.
    ...
    ...    Test Steps:

    [Tags]    ssi    ssi_dcs_event_start_stop    dcs    dcs_client    TEST_CASE_ID:2356228

    ${test_lib} =    Get Library Instance    dcs_dcp_test_lib
    Log To Console    \nExecuting DCS Client Start/Stop Test...
    ${test_result} =    Call Method    ${test_lib}    test_client_start_stop

    Should be True    ${test_result}