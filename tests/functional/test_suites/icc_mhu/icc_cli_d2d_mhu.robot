*** Settings ***
Documentation     Test suite to verify ICC MHU (Message Handling Unit) functionality between MCP and SCP across dies
...               1. The ICC_CLI_D2D_MHU functional test verifies bidirectional message communication 
...                  over channel 9 with message ID 2 and a 3-byte payload. When executed on MCP, 
...                  the communication occurs between MCP Die 0 and MCP Die 1; when run on SCP, 
...                  it occurs between SCP Die 0 and SCP Die 1.
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
Verify ICC MHU - ICC_CLI_D2D_MHU Functional Test
    [Documentation]    Verifies bidirectional message communication between MCP and SCP using ICC_CLI_D2D_MHU channel
    [Tags]    ssi    ssi_icc_mhu    ICC_CLI_D2D_MHU    TEST_CASE_ID:2615612

    # Define test parameters for D2D communication
    ${d2d_channel}=    Set Variable    ${9}    # ICC_CLI_D2D_MHU channel
    ${message_id}=    Set Variable    ${2}
    ${payload_size}=    Set Variable    ${3}
    @{payload_bytes}=    Create List    ${5}    ${6}    ${7}

    # Get test library instance
    ${test_lib}=    Get Library Instance    icc_mhu_test_lib

    # Test MCP_die0 to MCP_die1 communication
    ${mcp0_to_mcp1_result}=    Call Method    ${test_lib}    test_icc_cli_d2d_mhu    
    ...    sender_is_mcp=${TRUE}
    ...    sender_is_die0=${TRUE}
    ...    channel=${d2d_channel}
    ...    message_id=${message_id}
    ...    payload_size=${payload_size}
    ...    payload_bytes=${payload_bytes}
    Should Be True    ${mcp0_to_mcp1_result}    MCP_die0 to MCP_die1 communication test failed

    # Test MCP_die1 to MCP_die0 communication
    ${mcp1_to_mcp0_result}=    Call Method    ${test_lib}    test_icc_cli_d2d_mhu    
    ...    sender_is_mcp=${TRUE}
    ...    sender_is_die0=${FALSE}
    ...    channel=${d2d_channel}
    ...    message_id=${message_id}
    ...    payload_size=${payload_size}
    ...    payload_bytes=${payload_bytes}
    Should Be True    ${mcp1_to_mcp0_result}    MCP_die1 to MCP_die0 communication test failed

    # Test SCP_die0 to SCP_die1 communication
    ${scp0_to_scp1_result}=    Call Method    ${test_lib}    test_icc_cli_d2d_mhu    
    ...    sender_is_mcp=${FALSE}
    ...    sender_is_die0=${TRUE}
    ...    channel=${d2d_channel}
    ...    message_id=${message_id}
    ...    payload_size=${payload_size}
    ...    payload_bytes=${payload_bytes}
    Should Be True    ${scp0_to_scp1_result}    SCP_die0 to SCP_die1 communication test failed

    # Test SCP_die1 to SCP_die0 communication
    ${scp1_to_scp0_result}=    Call Method    ${test_lib}    test_icc_cli_d2d_mhu    
    ...    sender_is_mcp=${FALSE}
    ...    sender_is_die0=${FALSE}
    ...    channel=${d2d_channel}
    ...    message_id=${message_id}
    ...    payload_size=${payload_size}
    ...    payload_bytes=${payload_bytes}
    Should Be True    ${scp1_to_scp0_result}    SCP_die1 to SCP_die0 communication test failed

