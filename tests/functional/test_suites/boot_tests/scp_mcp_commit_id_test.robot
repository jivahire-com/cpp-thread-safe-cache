# Copyright (c) Microsoft Corporation. All rights reserved.
 
*** Settings ***
Documentation    Verify Commit SHA ID over Die0 UART and Build file
 
Library          ${CURDIR}/../../library/test_utility.py
...              name=scp_mcp_commit_id_test_logs
...              workspace_config=${WORKSPACE_CONFIG}
...              default_log_home=${LOG_DIR}
...              fw_payload_path=${PAYLOAD_DIR}
...              host_config=${HOST_CONFIG_DIR}/single_die_flash_boot.json
...              WITH NAME    scp_mcp_commit_id_test_lib

Suite Setup       Dut Setup        Die0
Suite Teardown    Dut Teardown
 
*** Variables ***
 
*** Test Cases ***
Test Case - SCP MCP Die0 Commit ID Comparison Check Test
    [Documentation]     SCP MCP Die0 Commit ID Comparison Check Test
 
    @{connections}=    Create List    scp0
    ${test_result}=    Compare Commit ID    ${connections}
    Should Be True    ${test_result}