# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies the Accel Fatal Interrupts in SCP

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../../pylibs
Library     pylibs.sdm_tests.accel_interrupt_test
...         name=accel_interrupt_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/cded_pipeline_fw.json
...         interrupt_config_path=${CURDIR}/accel_interrupts.json
...         WITH NAME    accel_interrupt_test_lib

*** Test Cases ***
Test Case - SCP: SDM PF Error Handling Test: FR [1423830]
    [Documentation]     Test the Accel Fatal Interrupts from SCP.

    # Get an instance of the accel_interrupt_test_lib library
    ${test_lib} =       Get Library Instance      accel_interrupt_test_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         accel_interrupt_test

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}