# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Die0: UEFI Boot Uptime Test [1140820]

Library          ${LIBRARIES}/test_utility.py
...              name=uefi_windows_boot_die1_logs
...              workspace_config=${WORKSPACE_CONFIG}
...              default_log_home=${LOG_DIR}
...              fw_payload_path=${PAYLOAD_DIR}
...              host_config=${HOST_CONFIG_DIR}/dual_die_flash_boot.json

Suite Setup       Dut Setup UEFI         DualDie
Suite Teardown    Dut Teardown

Test Tags        uefi_windows_boot_dual_die

*** Variables ***
@{SDM_UPTIME_PASS_RESPONSE}             Firmware Uptime Status: UP!
@{SDM_UPTIME_FAIL_RESPONSE}             Firmware Uptime Status: DOWN!
${SDM_UPTIME_TIMEOUT}                   ${20}
${IGNORE_NUMBERS}                       ${True}

*** Test Cases ***
Test Case - SDM: SDM Full Boot Chain - SDM Uptime Dual Die (Die 0) on UEFI Boot: [1140820]
    [Documentation]     Check SDM Uptime Dual Die (Die 0) on UEFI Boot
    
    Send Test Command     fw_info uptime       sdm0
    ${sdm0_test_result}    Parse Test Output    sdm0    Ok    
    ...    ${SDM_UPTIME_PASS_RESPONSE}    ${SDM_UPTIME_FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${SDM_UPTIME_TIMEOUT}
    Should Be True      ${sdm0_test_result}

Test Case - CDED: CDED Full Boot Chain - CDED Uptime Dual Die (Die 0) on UEFI Boot: [1140820]
    [Documentation]     Check CDED Uptime Dual Die (Die 0) on UEFI Boot

    Send Test Command      fw_info uptime       sdm_cded0
    ${cded0_test_result}    Parse Test Output    sdm_cded0    Ok    
    ...    ${SDM_UPTIME_PASS_RESPONSE}    ${SDM_UPTIME_FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${SDM_UPTIME_TIMEOUT}
    Should Be True      ${cded0_test_result}

Test Case - SDM: SDM Full Boot Chain - SDM Uptime Dual Die (Die 1) on UEFI Boot: [1140820]
    [Documentation]     Check SDM Uptime Dual Die (Die 1) on UEFI Boot

    Send Test Command     fw_info uptime       sdm1
    ${sdm1_test_result}    Parse Test Output    sdm1    Ok    
    ...    ${SDM_UPTIME_PASS_RESPONSE}    ${SDM_UPTIME_FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${SDM_UPTIME_TIMEOUT}
    Should Be True      ${sdm1_test_result}

Test Case - CDED: CDED Full Boot Chain - CDED Uptime Dual Die (Die 1) on UEFI Boot: [1140820]
    [Documentation]     Check CDED Uptime Dual Die (Die 1) on UEFI Boot

    Send Test Command      fw_info uptime       sdm_cded1
    ${cded1_test_result}    Parse Test Output    sdm_cded1    Ok    
    ...    ${SDM_UPTIME_PASS_RESPONSE}    ${SDM_UPTIME_FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${SDM_UPTIME_TIMEOUT}
    Should Be True      ${cded1_test_result}
