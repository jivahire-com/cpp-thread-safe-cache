# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Die0: UEFI Boot Uptime Test [1140820]

Library          ${LIBRARIES}/test_utility.py
...              name=uefi_windows_boot_die0_logs
...              workspace_config=${WORKSPACE_CONFIG}
...              default_log_home=${LOG_DIR}
...              fw_payload_path=${PAYLOAD_DIR}
...              host_config=${HOST_CONFIG_DIR}/single_die_flash_boot.json

Suite Setup       Dut Setup UEFI        Die0
Suite Teardown    Dut Teardown

Test Tags        uefi_windows_boot_single_die

*** Variables ***
@{SDM_FBC_PASS_RESPONSE}            Firmware Uptime Status: UP!
@{SDM_FBC_FAIL_RESPONSE}            Firmware Uptime Status: DOWN!
${SDM_FBC_TIMEOUT}                  ${20}

${IGNORE_NUMBERS}                   ${True}

*** Test Cases ***
Test Case - SDM: SDM Full Boot Chain - SDM Uptime Single Die (Die 0) on UEFI Boot: [1140820]
    [Documentation]     Check SDM Uptime Single Die (Die 0) on UEFI Boot
    
    Send Test Command     fw_info uptime       sdm0
    ${sdm_test_result}    Parse Test Output    sdm0    Ok    
    ...    ${SDM_FBC_PASS_RESPONSE}    ${SDM_FBC_FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${SDM_FBC_TIMEOUT}
    Should Be True      ${sdm_test_result}

Test Case - CDED: CDED Full Boot Chain - CDED Uptime Single Die (Die 0) on UEFI Boot: [1140820]
    [Documentation]     Check CDED Uptime Single Die (Die 0) on UEFI Boot (Die 0)

    Send Test Command      fw_info uptime       sdm_cded0
    ${cded_test_result}    Parse Test Output    sdm_cded0    Ok    
    ...    ${SDM_FBC_PASS_RESPONSE}    ${SDM_FBC_FAIL_RESPONSE}    ${IGNORE_NUMBERS}    ${SDM_FBC_TIMEOUT}
    Should Be True      ${cded_test_result}

