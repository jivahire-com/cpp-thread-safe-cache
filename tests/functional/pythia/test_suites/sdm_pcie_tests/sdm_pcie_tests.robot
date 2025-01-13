# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies the enumeration of SDM and CDED cores from AP core 0.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.kng_pythia_libs.APBaremetalTests
...         name=Sdm_Pcie_Tests_log
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/ap_baremetal_embed_fw_single_die.json
...         WITH NAME    APBaremetalTestsLib

Suite Setup       Dut Setup
Suite Teardown    Dut Teardown


*** Variables ***
@{PCIE_PASS_RESPONSE}              Starting PCIe Test
...                                PCIe Test DONE

@{SDM_MEMCPY_PASS_RESPONSE}        wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                compare_data_buffer: INFO: SUCCESS
...                                sdm_memcpy_test-: ****  SDM MEMCPY Test on Die 0 DONE ****
@{E2I_I2E_PASS_RESPONSE}           User Defined Loopback Test on Die 0 End!
@{FAIL_RESPONSE}                   FAIL
${TIMEOUT}                         ${80}

*** Test Cases ***
Test Case - AP: Test PCIe Enumeration Test: [1423999]
    [Documentation]     Test the PCIe enumeration of SDM and CDED cores from AP core 0.
    Send Test Command    ap_bm pcie_test 
    ${test_result}    Parse Test Output    PCIe Test DONE     ${PCIE_PASS_RESPONSE}      
    ...    ${FAIL_RESPONSE}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: SDM memcpy Test: FR [1423800]
    [Documentation]     Test the SDM memcpy on SDM from AP core 0.
    Send Test Command    ap_bm sdm_memcpy_test 0 
    ${test_result}    Parse Test Output    SDM MEMCPY Test on Die 0 DONE     ${SDM_MEMCPY_PASS_RESPONSE}      
    ...    ${FAIL_RESPONSE}    ${300}
    Should Be True      ${test_result}

Test Case - AP: SDM E2I I2E Test: FR [1738499]
    [Documentation]     Test the SDM E2I I2E of SDM from AP core 0.
    Send Test Command    ap_bm user_def_loopback_test 0 0 0 
    ${test_result}    Parse Test Output    User Defined Loopback Test on Die 0 End!     ${E2I_I2E_PASS_RESPONSE}      
    ...    ${FAIL_RESPONSE}    ${TIMEOUT}
    Should Be True      ${test_result}