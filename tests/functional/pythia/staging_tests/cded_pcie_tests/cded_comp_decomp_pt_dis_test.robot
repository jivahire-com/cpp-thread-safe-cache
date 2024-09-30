# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Test the CDED Compress (verified by Decompress) and Decompress functionality from AP (Core 0) when passthrough is disabled.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.cded_tests.cded_comp_decomp_pt_dis_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    cded_comp_decomp_pt_dis_test_lib

*** Variables ***
@{PASS_RESPONSE}        Starting  CDED Test
...                     accelerator_id = Accel-CDED (1)
...                     workload: opcode = CMP_DCM (0x9)
...                     wait_for_cmd_completion-: INFO: Status: exp=0x0, rx=0x0
...                     wait_for_cmd_completion-: INFO: RX: SUCCESS
...                     Both CRCs are same.
...                     compare_data_buffer: INFO: SUCCESS
...                     CDED Test DONE

@{FAIL_RESPONSE}        FAIL

*** Test Cases ***
Test Case - AP: CDED PF Compression-Decompression Test with PassThrough Disabled: [1836729]
    [Documentation]     Test the CDED Compress (verified by Decompress) and Decompress functionality from AP (Core 0).

    # Get an instance of the cded_comp_decomp_pt_dis_test library
    ${test_lib} =       Get Library Instance      cded_comp_decomp_pt_dis_test_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         cded_comp_decomp_pt_dis_test       ${PASS_RESPONSE}        ${FAIL_RESPONSE}

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}