# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Test the CDED the VF functionality completes from AP (Core 0)

Library     ${CURDIR}/../../pylibs
Library     pylibs.kng_pythia_libs.APBaremetalTests
...         name=cded_vf_tests_logs
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    APBaremetalTestsLib

Suite Setup       Environment Setup
Suite Teardown    Test Teardown


*** Variables ***
@{PASS_RESPONSE}        INFO: Status: exp=0x0, rx=0x0
...                     INFO: SUCCESS   
@{FAIL_RESPONSE}        FAIL
${COMMAND_CDED_DIE0_VF1_COMP_DECOMP}          ap_bm cded_test -o cx -a zlib -v
${COMMAND_CDED_DIE0_VF1_ENCR_DECR}            ap_bm cded_test -o xed -v
${READ UNTIL}           CDED Test DONE


*** Test Cases ***
Test Case - AP: CDED Single VF Compress-Decompress Test: [1615866]
    [Documentation]     Test the CDED compress decompress single VF functionality completes from AP (Core 0).
    Send Test Command    ${COMMAND_CDED_DIE0_VF1_COMP_DECOMP}
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE}        
    ...    ${FAIL_RESPONSE}
    Should Be True      ${test_result}

Test Case - AP: CDED Single VF Encryption-Decryption Test: [1615866]
    [Documentation]     Test the CDED encryption decryption single VF functionality completes from AP (Core 0).
    Send Test Command    ${COMMAND_CDED_DIE0_VF1_ENCR_DECR}
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE}        
    ...    ${FAIL_RESPONSE}
    Should Be True      ${test_result}


*** Keywords ***
Environment Setup
   ${test_lib}    Get Library Instance      APBaremetalTestsLib
   ${connection}    Test Setup
    