# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies warm start CLI commands.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.cold_start_tests.cold_start_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    cold_start_test_lib

*** Test Cases ***
Test Case - Cold start CLI Test
    # Get an instance of the cold_start_test library
    ${test_lib} =      Get Library Instance    cold_start_test_lib

    # Call the test method from the test library and store the result
    ${test_result} =    Call Method     ${test_lib}     cold_start_test

    # Log the status
    Should Be True      ${test_result}
