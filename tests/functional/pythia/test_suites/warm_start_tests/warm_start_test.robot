# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies warm start CLI commands.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.warm_start_tests.warm_start_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw.json
...         WITH NAME    warm_start_test_lib

*** Test Cases ***
Test Case - Warm start CLI Test
    [Documentation]     Verifies warm start CLI commands.

    # Get an instance of the warm_start_test library
    ${test_lib} =       Get Library Instance      warm_start_test_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         warm_start_test

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}