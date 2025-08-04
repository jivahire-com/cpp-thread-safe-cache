*** Settings ***
Documentation     Test suite to verify ICC MHU (Message Handling Unit) functionality between MCP and SCP on same die
...               1. ICC_CLI_MSCP_MHU Communication Test: Verifies bidirectional message communication
...                  between MCP and SCP using ICC MHU channel 4 with message ID 2 and 3-byte payload.
Library    ${CURDIR}/../../library

Library    library.icc_mhu_tests.icc_mhu_lib_test.IccMhuTest
...    workspace_config=${WORKSPACE_CONFIG}
...    default_log_home=${LOG_DIR}
...    fw_payload_path=${PAYLOAD_DIR}
...    host_config=${HOST_CONFIG_DIR}/${HOST_JSON}
...    WITH NAME    icc_mhu_test_lib

Suite Setup       Setup Test Environment
Suite Teardown    Teardown Test Environment

*** Keywords ***
Setup Test Environment
    ${test_lib}=    Get Library Instance    icc_mhu_test_lib
    ${setup_result}=    Call Method    ${test_lib}    setup_dut
    Should Be True    ${setup_result}    Failed to setup DUT

Teardown Test Environment
    ${test_lib}=    Get Library Instance    icc_mhu_test_lib
    Run Keyword And Ignore Error    Call Method    ${test_lib}    teardown_dut

*** Test Cases ***
Verify ICC MHU - ICC_CLI_MSCP_MHU Communication Between MCP and SCP
    [Documentation]    Verifies bidirectional message communication between MCP and SCP using ICC MHU
    [Tags]    ssi    ssi_icc_mhu    ICC_CLI_MSCP_MHU    TEST_CASE_ID:2614934

    # Define test parameters
    ${mhu_channel}=    Set Variable    ${4}
    ${message_id}=    Set Variable    ${2}
    ${payload_size}=    Set Variable    ${3}
    @{payload_bytes}=    Create List    ${5}    ${6}    ${7}

    # Get test library instance
    ${test_lib}=    Get Library Instance    icc_mhu_test_lib

    # Test MCP to SCP communication
    ${mcp_to_scp_result}=    Call Method    ${test_lib}    test_icc_cli_mscp_mhu    
    ...    sender_is_mcp=${TRUE}
    ...    channel=${mhu_channel}
    ...    message_id=${message_id}
    ...    payload_size=${payload_size}
    ...    payload_bytes=${payload_bytes}
    Should Be True    ${mcp_to_scp_result}    MCP to SCP communication test failed

    # Test SCP to MCP communication
    ${scp_to_mcp_result}=    Call Method    ${test_lib}    test_icc_cli_mscp_mhu    
    ...    sender_is_mcp=${FALSE}
    ...    channel=${mhu_channel}
    ...    message_id=${message_id}
    ...    payload_size=${payload_size}
    ...    payload_bytes=${payload_bytes}
    Should Be True    ${scp_to_mcp_result}    SCP to MCP communication test failed

