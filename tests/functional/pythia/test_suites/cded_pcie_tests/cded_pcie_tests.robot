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
...         name=Cded_Pcie_Tests_log
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/cded_pipeline_fw.json
...         WITH NAME    APBaremetalTestsLib

Suite Setup       Dut Setup
Suite Teardown    Dut Teardown


*** Variables ***
@{FAIL_RESPONSE}        FAIL
${READ UNTIL}           CDED Test DONE
${TIMEOUT}              ${300}
@{PASS_RESPONSE_AES_GCM_ENCR_DECR}      Starting  CDED Test
...                                     accelerator_id = Accel-CDED (1)
...                                     workload: opcode = AES-GCM_AES-GCM (0x21)
...                                     wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                     wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                     Both CRCs are same.
...                                     CDED Test DONE

@{PASS_RESPONSE_AES_XTS_ENCR_DECR_PASSTHROUGH}        accelerator_id = Accel-CDED (1)
...                                                   workload: opcode = AES-XTS_AES-XTS (0x20)
...                                                   wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                                   wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                                   Both CRCs are same.

@{PASS_RESPONSE_AES_XTS_ENCR_DECR_NO_PASSTHROUGH}   Starting  CDED Test
...                                                 accelerator_id = Accel-CDED (1)
...                                                 workload: opcode = AES-XTS_AES-XTS (0x20)
...                                                 wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                                 wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                                 Both CRCs are same.
...                                                 CDED Test DONE

@{PASS_RESPONSE_COMP_DECOMP_NO_PASSTHROUGH}        Starting  CDED Test
...                                                accelerator_id = Accel-CDED (1)
...                                                workload: opcode = CMP_DCM (0x9)
...                                                wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                                wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                                Both CRCs are same.
...                                                compare_data_buffer: INFO: SUCCESS
...                                                CDED Test DONE

@{PASS_RESPONSE_COMP_DECOMP_PASSTHROUGH}       Starting  CDED Test
...                                            accelerator_id = Accel-CDED (1)
...                                            workload: opcode = CMP_DCM (0x9)
...                                            wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                            wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                            Both CRCs are same.
...                                            CDED Test DONE

@{PASS_RESPONSE_COMP_DECOMP}        compare_data_buffer: INFO: SUCCESS
...                                 wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0

@{PASS_RESPONSE_DECOMP_ONLY}          Starting  CDED Test
...                                   accelerator_id = Accel-CDED (1)
...                                   workload: opcode = Decompression (0x6)
...                                   wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                   wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                   Received valid data.
...                                   CDED Test DONE

*** Test Cases ***
Test Case - AP: CDED PF GCM Encryption-Decryption test with Keys in DDR: [1836729]
    [Documentation]     Test the CDED AES-GCM Encrypt/Decrypt functionality from AP (Core 0)    
    Send Test Command    ap_bm cded_test -o ged
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE_AES_GCM_ENCR_DECR}      
    ...    ${FAIL_RESPONSE}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED PF XTS Encryption-Decryption test with PassThrough Enabled (Keys in DDR): [1836729]
    [Documentation]     Test the CDED AES-XTS Encrypt/Decrypt functionality with passthrough enabled from AP (Core 0)
    Send Test Command    ap_bm cded_test -o xed -p
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE_AES_XTS_ENCR_DECR_PASSTHROUGH}      
    ...    ${FAIL_RESPONSE}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED PF XTS Encryption-Decryption test with Keys in DDR: [1836729]
    [Documentation]     Test the CDED AES-XTS Encrypt/Decrypt functionality from AP (Core 0)
    Send Test Command    ap_bm cded_test -o xed
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE_AES_XTS_ENCR_DECR_NO_PASSTHROUGH}      
    ...    ${FAIL_RESPONSE}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED PF Compression-Decompression Test with PassThrough Disabled: [1836729]
    [Documentation]     Test the CDED Compress (verified by Decompress) and Decompress functionality from AP (Core 0).
    Send Test Command    ap_bm cded_test -o cx
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE_COMP_DECOMP_NO_PASSTHROUGH}      
    ...    ${FAIL_RESPONSE}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED PF Compression-Decompression Test with PassThough Enabled: [1836729]
    [Documentation]     Test the CDED Compress (verified by Decompress) and Decompress functionality from AP (Core 0) when passthrough is enabled.
    Send Test Command    ap_bm cded_test -o cx -p
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE_COMP_DECOMP_PASSTHROUGH}      
    ...    ${FAIL_RESPONSE}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED PF Compress-Decompress Test: [1836729]
    [Documentation]     Test the CDED compression and Decompression completes from AP core 0.
    Send Test Command    ap_bm cded_test
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE_COMP_DECOMP}      
    ...    ${FAIL_RESPONSE}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED PF Decompression Test: [1836729]
    [Documentation]     Test the CDED Compress (verified by Decompress) and Decompress functionality from AP (Core 0).
    Send Test Command    ap_bm cded_test -o x
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE_DECOMP_ONLY}      
    ...    ${FAIL_RESPONSE}    ${TIMEOUT}
    Should Be True      ${test_result}
