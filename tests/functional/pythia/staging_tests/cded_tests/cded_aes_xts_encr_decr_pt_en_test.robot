# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Test the CDED AES-XTS Encrypt/Decrypt functionality with passthrough enabled from AP (Core 0)

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.cded_tests.cded_aes_xts_encr_decr_pt_en_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    cded_aes_xts_encr_decr_pt_en_test_lib

*** Variables ***
@{PASS_RESPONSE}        accelerator_id = Accel-CDED (1)
...                     workload: opcode = AES-XTS_AES-XTS (0x20)
...                     wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                     wait_for_cmd_completion-: INFO: RX: SUCCESS
...                     Both CRCs are same.

@{FAIL_RESPONSE}        FAIL

*** Test Cases ***
Test Case - AP: CDED PF XTS Encryption-Decryption test with PassThrough Enabled (Keys in DDR): [1836729]
    [Documentation]     Test the CDED AES-XTS Encrypt/Decrypt functionality with passthrough enabled from AP (Core 0)

    # Get an instance of the cded_aes_xts_encr_decr_pt_en_test library
    ${test_lib} =       Get Library Instance      cded_aes_xts_encr_decr_pt_en_test_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         cded_aes_xts_encr_decr_pt_en_test       ${PASS_RESPONSE}        ${FAIL_RESPONSE}

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}