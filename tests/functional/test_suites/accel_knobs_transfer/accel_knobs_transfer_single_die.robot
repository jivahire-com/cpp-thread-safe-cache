# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Die0: Accel (SDM / CDED): Verify Accel Knobs Transfer

Library          ${LIBRARIES}/test_utility.py
...              name=accel_knobs_transfer_single_die_logs
...              workspace_config=${WORKSPACE_CONFIG}
...              default_log_home=${LOG_DIR}
...              fw_payload_path=${PAYLOAD_DIR}
...              host_config=${HOST_CONFIG_DIR}/single_die_flash_boot.json

Suite Setup       Dut Setup APBM        Die0
Suite Teardown    Dut Teardown

Test Tags        accel_knobs_transfer_single_die

*** Variables ***
@{DIE0_PASS_RESPONSE}               Die ID: --0x0--
@{FAIL_RESPONSE}                    Die ID: --0x2--
${TIMEOUT}                          ${20}
${IGNORE_NUMBERS}                   ${True}

*** Test Cases ***
Test Case - SDM: SDM Full Boot Chain - SDM Knobs Transfer Single Die (Die 0)
    [Documentation]     Check SDM Knobs Transfer Single Die (Die 0)
    
    Send Test Command     fw_info whoami       sdm0
    ${sdm_test_result}    Parse Test Output    sdm0    Ok    
    ...    ${DIE0_PASS_RESPONSE}    ${FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${sdm_test_result}

Test Case - CDED: CDED Full Boot Chain - CDED Knobs Transfer Single Die (Die 0)
    [Documentation]     Check CDED Knobs Transfer Single Die (Die 0)

    Send Test Command      fw_info whoami       sdm_cded0
    ${cded_test_result}    Parse Test Output    sdm_cded0    Ok    
    ...    ${DIE0_PASS_RESPONSE}    ${FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${cded_test_result}

