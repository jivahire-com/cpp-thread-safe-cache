# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Die1: CDED: Descriptors submission on PF (x1) from AP (Core 0) [1836729].

Library          ${LIBRARIES}/test_utility.py
...              name=cded_pcie_die1_tests_logs
...              workspace_config=${WORKSPACE_CONFIG}
...              default_log_home=${LOG_DIR}
...              fw_payload_path=${PAYLOAD_DIR}
...              host_config=${HOST_CONFIG_DIR}/dual_die_flash_boot.json

Suite Setup       Dut Setup APBM        DualDie
Suite Teardown    Dut Teardown

Test Tags         cded_descriptors_die1_tests

*** Variables ***
${IGNORE_NUMBERS}       ${True}
@{FAIL_RESPONSE}        FAIL
${READ UNTIL}           CDED Test DONE
${TIMEOUT}              ${900}
@{PASS_RESPONSE_AES_GCM_ENCR_DECR}      Starting  CDED Test
...                                     accelerator_id = Accel-CDED (1)
...                                     workload: opcode = AES-GCM_AES-GCM (0x420)
...                                     wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                     wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                     Both CRCs are same.
...                                     CDED Test DONE

@{PASS_RESPONSE_AES_XTS_ENCR_DECR_PASSTHROUGH}        accelerator_id = Accel-CDED (1)
...                                                   workload: opcode = AES-XTS_AES-XTS (0x418)
...                                                   wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                                   wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                                   Both CRCs are same.

@{PASS_RESPONSE_AES_XTS_ENCR_DECR_NO_PASSTHROUGH}   Starting  CDED Test
...                                                 accelerator_id = Accel-CDED (1)
...                                                 workload: opcode = AES-XTS_AES-XTS (0x418)
...                                                 wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                                 wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                                 Both CRCs are same.
...                                                 CDED Test DONE

@{PASS_RESPONSE_COMP_DECOMP_NO_PASSTHROUGH}        Starting  CDED Test
...                                                accelerator_id = Accel-CDED (1)
...                                                workload: opcode = CMP_DCM (0x4C0)
...                                                wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                                wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                                Both CRCs are same.
...                                                compare_data_buffer: INFO: SUCCESS
...                                                CDED Test DONE

@{PASS_RESPONSE_COMP_DECOMP_PASSTHROUGH}       Starting  CDED Test
...                                            accelerator_id = Accel-CDED (1)
...                                            workload: opcode = CMP_DCM (0x4C0)
...                                            wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                            wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                            Both CRCs are same.
...                                            CDED Test DONE

@{PASS_RESPONSE_COMP_DECOMP}          compare_data_buffer: INFO: SUCCESS
...                                   wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0

@{PASS_RESPONSE_DECOMP_ONLY}          Starting  CDED Test
...                                   accelerator_id = Accel-CDED (1)
...                                   workload: opcode = Decompression (0x440)
...                                   wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                                   wait_for_cmd_completion-: INFO: RX: SUCCESS
...                                   CDED Test DONE

@{PASS_RESPONSE_USER_DEF_OPCODE}       Starting  CDED Test
...                                    accelerator_id = Accel-CDED (1)
...                                    workload: opcode = USER_DEF_OPCODE (0x3C4)
...                                    wait_for_cmd_completion-464: INFO: Status: exp=0x0, rx=0x0
...                                    wait_for_cmd_completion-476: INFO: RX: SUCCESS
...                                    CDED Test DONE

*** Test Cases ***
Test Case - AP: CDED Die1 PF GCM Encryption-Decryption test with Keys in DDR: [1836729]
    [Documentation]     Test the CDED Die1 AES-GCM Encrypt/Decrypt functionality from AP (Core 0)  
    Send Test Command    ap_bm cded_test -o ged -d 1    apns0
    ${test_result}    Parse Test Output    apns0    ${READ UNTIL}     ${PASS_RESPONSE_AES_GCM_ENCR_DECR}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED Die1 PF XTS Encryption-Decryption test with PassThrough Enabled (Keys in DDR): [1836729]
    [Documentation]     Test the CDED Die1 AES-XTS Encrypt/Decrypt functionality with passthrough enabled from AP (Core 0)
    Send Test Command    ap_bm cded_test -o xed -p -d 1    apns0
    ${test_result}    Parse Test Output    apns0    ${READ UNTIL}     ${PASS_RESPONSE_AES_XTS_ENCR_DECR_PASSTHROUGH}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED Die1 PF XTS Encryption-Decryption test with Keys in DDR: [1836729]
    [Documentation]     Test the CDED Die1 AES-XTS Encrypt/Decrypt functionality from AP (Core 0)
    Send Test Command    ap_bm cded_test -o xed -d 1    apns0
    ${test_result}    Parse Test Output    apns0    ${READ UNTIL}     ${PASS_RESPONSE_AES_XTS_ENCR_DECR_NO_PASSTHROUGH}   
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED Die1 PF Compression-Decompression Test with PassThrough Disabled: [1836729]
    [Documentation]     Test the CDED Die1 Compress (verified by Decompress) and Decompress functionality from AP (Core 0).
    Send Test Command    ap_bm cded_test -o cx -d 1    apns0
    ${test_result}    Parse Test Output    apns0    ${READ UNTIL}     ${PASS_RESPONSE_COMP_DECOMP_NO_PASSTHROUGH}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED Die1 PF Compression-Decompression Test with PassThough Enabled: [1836729]
    [Documentation]     Test the CDED Die1 Compress (verified by Decompress) and Decompress functionality from AP (Core 0) when passthrough is enabled.
    Send Test Command    ap_bm cded_test -o cx -p -d 1    apns0
    ${test_result}    Parse Test Output    apns0    ${READ UNTIL}     ${PASS_RESPONSE_COMP_DECOMP_PASSTHROUGH}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED Die1 PF Compress-Decompress Test: [1836729]
    [Documentation]     Test the CDED Die1 compression and Decompression completes from AP core 0.
    Send Test Command    ap_bm cded_test -d 1 -o cx    apns0
    ${test_result}    Parse Test Output    apns0    ${READ UNTIL}     ${PASS_RESPONSE_COMP_DECOMP}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED Die1 PF Decompression Test: [1836729]
    [Documentation]     Test the CDED Die1 Compress (verified by Decompress) and Decompress functionality from AP (Core 0).
    Send Test Command    ap_bm cded_test -o x -d 1    apns0
    ${test_result}    Parse Test Output    apns0    ${READ UNTIL}     ${PASS_RESPONSE_DECOMP_ONLY}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}

Test Case - AP: CDED Die1 User Define Opcode Test: [1931002]
    [Documentation]     Test the CDED Die0 User define opcode functionality from AP (Core 0)
    Send Test Command    ap_bm cded_test -o ud_op -d 1    apns0
    ${test_result}    Parse Test Output    apns0    ${READ UNTIL}     ${PASS_RESPONSE_USER_DEF_OPCODE}
    ...    ${FAIL_RESPONSE}     ${IGNORE_NUMBERS}    ${TIMEOUT}
    Should Be True      ${test_result}