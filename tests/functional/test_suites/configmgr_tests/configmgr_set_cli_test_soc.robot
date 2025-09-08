# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies Config Manager CLI Set command.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.configmgr_tests.configmgr_set_cli_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/debugpc-1103e12-n1.json
...         WITH NAME    configmgr_set_cli_test_lib

*** Test Cases ***
Test Case - Config Manager CLI Set Test
    [Documentation]     Verifies Config Manager CLI Set command.

    # Get an instance of the configmgr_set_cli_test_lib library
    ${test_lib} =       Get Library Instance      configmgr_set_cli_test_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         configmgr_set_cli_test

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}