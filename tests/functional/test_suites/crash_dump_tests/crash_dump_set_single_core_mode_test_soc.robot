# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies crash dump set single core mode

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.crash_dump_tests.crash_dump_set_single_core_mode_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    crash_dump_set_single_core_mode_test_lib

*** Test Cases ***
Test Case - crash dump set single core mode test.
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    crash_dump_set_single_core_mode_test_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    crash_dump_set_single_core_mode_test

    # Log the status
    Should be True    ${test_result}
