# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies AP WDTimeout error handling.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.ap_wdt_test.ap_wdt_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    ap_wdt_test_lib

*** Test Cases ***
Test Case - AP WDT Error Handling Test
    [Documentation]     Verifies AP WDTimeout error handling.

    # Get an instance of the warm_start_test library
    ${test_lib} =       Get Library Instance      ap_wdt_test_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         ap_wdt_test

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}