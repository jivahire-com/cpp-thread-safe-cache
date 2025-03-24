# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Die0: ICC/MBOX: SVP: Large FIFO Mailbox SEND/RECV command for CDED <-> MCP [2258003].

Library          ${LIBRARIES}/test_utility.py
...              name=mcp_cded_mbx_die0_tests_logs
...              workspace_config=${WORKSPACE_CONFIG}
...              default_log_home=${LOG_DIR}
...              fw_payload_path=${PAYLOAD_DIR}
...              host_config=${HOST_CONFIG_DIR}/single_die_flash_boot.json

Suite Setup       Dut Setup APBM        Die0
Suite Teardown    Dut Teardown

*** Variables ***
@{INIT_RECV_PASS_RESPONSE}          Recv Initiated: Status[0x0]
@{RECV_PASS_RESPONSE}               Recv Complete: Status[0x0]
@{SEND_PASS_RESPONSE}               Send Complete: Status[0x0]
@{FAIL_RESPONSE}                    FAIL
${TIMEOUT}                          ${15}
${IGNORE_NUMBERS}                   ${False}  # Set this to True to ignore numbers, False to consider them

*** Test Cases ***
Test Case - ICC Loopback Test CDED Send MCP Receive: [2116490]

    Send Test Command    icc_largembx cded_recv 2    mcp0
    ${mcp_test_result}    Parse Test Output    mcp0    Recv Initiated: Status[0x0]    
    ...    ${INIT_RECV_PASS_RESPONSE}    ${FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${mcp_test_result}
    
    Send Test Command    mcp_mbx mcp_send 2    sdm_cded0
    ${cded_test_result}    Parse Test Output    sdm_cded0    Send Complete: Status[0x0]    
    ...    ${SEND_PASS_RESPONSE}    ${FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${cded_test_result}

    ${mcp_test_result}    Parse Test Output    mcp0    Recv Complete: Status[0x0]    
    ...    ${RECV_PASS_RESPONSE}    ${FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${mcp_test_result}


Test Case - ICC Loopback Test MCP Send CDED Receive: [2116490]

    Send Test Command    mcp_mbx mcp_recv 3    sdm_cded0
    ${cded_test_result}    Parse Test Output    sdm_cded0      Recv Initiated: Status[0x0]     ${INIT_RECV_PASS_RESPONSE}      
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${cded_test_result}
    
    Send Test Command    icc_largembx cded_send 3    mcp0

    ${mcp_test_result}    Parse Test Output    mcp0      Send Complete: Status[0x0]     ${SEND_PASS_RESPONSE}      
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${mcp_test_result}

    ${cded_test_result}    Parse Test Output    sdm_cded0      Recv Complete: Status[0x0]     ${RECV_PASS_RESPONSE}      
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${cded_test_result}
