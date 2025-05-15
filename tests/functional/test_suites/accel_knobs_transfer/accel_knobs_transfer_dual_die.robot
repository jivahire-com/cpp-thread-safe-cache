# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Die0: Accel (SDM / CDED): Verify Accel Knobs Transfer

Library          ${LIBRARIES}/test_utility.py
...              name=accel_knobs_transfer_dual_die_logs
...              workspace_config=${WORKSPACE_CONFIG}
...              default_log_home=${LOG_DIR}
...              fw_payload_path=${PAYLOAD_DIR}
...              host_config=${HOST_CONFIG_DIR}/dual_die_flash_boot.json

Suite Setup       Dut Setup APBM        DualDie
Suite Teardown    Dut Teardown

Test Tags        accel_knobs_transfer_dual_die

*** Variables ***
@{DIE0_PASS_RESPONSE}               Die ID: --0x0--
@{DIE1_PASS_RESPONSE}               Die ID: --0x1--
@{FAIL_RESPONSE}                    Die ID: --0x2--
${TIMEOUT}                          ${20}
${IGNORE_NUMBERS}                   ${True}

*** Test Cases ***
Test Case - SDM: SDM Full Boot Chain - SDM Knobs Transfer Dual Die (Die 0)
    [Documentation]     Check SDM Knobs Transfer Dual Die (Die 0)
    
    Send Test Command     fw_info whoami       sdm0
    ${sdm0_test_result}    Parse Test Output    sdm0    Ok    
    ...    ${DIE0_PASS_RESPONSE}    ${FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${sdm0_test_result}

Test Case - AP: CDED Full Boot Chain - CDED Knobs Transfer Dual Die (Die 0)
    [Documentation]     Check CDED Knobs Transfer Dual Die (Die 0)

    Send Test Command      fw_info whoami       sdm_cded0
    ${cded0_test_result}    Parse Test Output    sdm_cded0    Ok    
    ...    ${DIE0_PASS_RESPONSE}    ${FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${cded0_test_result}

Test Case - SDM: SDM Full Boot Chain - SDM Knobs Transfer Dual Die (Die 1)
    [Documentation]     Check SDM Knobs Transfer Dual Die (Die 1)

    Send Test Command     fw_info whoami       sdm1
    ${sdm1_test_result}    Parse Test Output    sdm1    Ok    
    ...    ${DIE1_PASS_RESPONSE}    ${FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${sdm1_test_result}

Test Case - CDED: CDED Full Boot Chain - CDED Knobs Transfer Dual Die (Die 1)
    [Documentation]     Check CDED Knobs Transfer Dual Die (Die 1)

    Send Test Command      fw_info whoami       sdm_cded1
    ${cded1_test_result}    Parse Test Output    sdm_cded1    Ok    
    ...    ${DIE1_PASS_RESPONSE}    ${FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${cded1_test_result}
