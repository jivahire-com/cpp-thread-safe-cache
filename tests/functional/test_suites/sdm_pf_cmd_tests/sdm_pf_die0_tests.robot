# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Die1: SDM: PCIe Enumeration of PF (RCiEP / RCEC) and all descriptor submission on PF (x1) from AP (Core 0) [1423999]

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().

Library          ${LIBRARIES}/test_utility.py
...              name=sdm_pcie_die0_tests_logs
...              workspace_config=${WORKSPACE_CONFIG}
...              default_log_home=${LOG_DIR}
...              fw_payload_path=${PAYLOAD_DIR}
...              host_config=${HOST_CONFIG_DIR}/single_die_flash_boot.json

Suite Setup       Dut Setup APBM        Die0
Suite Teardown    Dut Teardown

Test Tags         sdm_pf_die0_tests

*** Variables ***
@{PCIE_PASS_RESPONSE}               Starting PCIe Test
...                                 PCIe Test DONE

@{SDM_MEMCPY_PASS_RESPONSE}         wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                 compare_data_buffer: INFO: SUCCESS
...                                 sdm_memcpy_test-: ****  SDM MEMCPY Test on Die 0 DONE ****
@{E2I_I2E_PASS_RESPONSE}            User Defined Loopback Test on Die 0 End!
@{INV_OPCODE_TEST_PASS_RESPONSE}    User Defined Invalid Opcode Test on Die 0 End!
@{FAIL_RESPONSE}                    FAIL
${TIMEOUT}                          ${80}
${IGNORE_NUMBERS}                   ${True}  # Set this to True to ignore numbers, False to consider them


*** Test Cases ***
Test Case - AP: Test PCIe Enumeration Test: [1423999]
    [Documentation]     Test the PCIe enumeration of SDM and CDED cores from AP core 0.
    [Tags]    smoke
    Send Test Command    ap_bm pcie_test    apns0
    ${test_result}    Parse Test Output    apns0    PCIe Test DONE     ${PCIE_PASS_RESPONSE}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: SDM memcpy Test: FR [1423800]
    [Documentation]     Test the SDM memcpy on SDM from AP core 0.

    Send Test Command    ap_bm sdm_memcpy_test 0    apns0
    ${test_result}    Parse Test Output    apns0    SDM MEMCPY Test on Die 0 DONE     ${SDM_MEMCPY_PASS_RESPONSE} 
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${300}
    Should Be True      ${test_result}

Test Case - AP: SDM E2I I2E Test: FR [1738499]
    [Documentation]     Test the SDM E2I I2E of SDM from AP core 0.

    Send Test Command    ap_bm user_def_loopback_test 0 0 0    apns0 
    ${test_result}    Parse Test Output    apns0    User Defined Loopback Test on Die 0 End!     ${E2I_I2E_PASS_RESPONSE}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

# Running user_def_loopback_test and user_def_invalid_opcode_test again to check for qork queue being clearted after command submission
Test Case - AP: SDM User Defined Invalid OpCode Test: FR [1738499]
    [Documentation]     Test the SDM User Defined Invalid Opcode Command of SDM from AP core 0.

    Send Test Command    ap_bm user_def_invalid_opcode_test 0 0 0    apns0 
    ${test_result}    Parse Test Output    apns0     User Defined Invalid Opcode Test on Die 0 End!     ${INV_OPCODE_TEST_PASS_RESPONSE}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: SDM E2I I2E Test: FR [1738499]
    [Documentation]     Test the SDM E2I I2E of SDM from AP core 0.

    Send Test Command    ap_bm user_def_loopback_test 0 0 0    apns0 
    ${test_result}    Parse Test Output    apns0    User Defined Loopback Test on Die 0 End!     ${E2I_I2E_PASS_RESPONSE}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: SDM User Defined Invalid OpCode Test: FR [1738499]
    [Documentation]     Test the SDM User Defined Invalid Opcode Command of SDM from AP core 0.

    Send Test Command    ap_bm user_def_invalid_opcode_test 0 0 0    apns0 
    ${test_result}    Parse Test Output    apns0     User Defined Invalid Opcode Test on Die 0 End!     ${INV_OPCODE_TEST_PASS_RESPONSE}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}