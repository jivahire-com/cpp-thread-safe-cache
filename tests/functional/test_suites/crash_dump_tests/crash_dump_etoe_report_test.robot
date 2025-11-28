# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies crash_dump_etoe_report_test functionality on the KingsgateSOC platform.
Suite Setup      Setup Suite
Suite Teardown   Teardown Suite

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.crash_dump_tests.crash_dump_etoe_report_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    crash_dump_etoe_report_test_lib

*** Variables ***
${Success}    ${True}
${Failure}    ${False}

*** Keywords *** 
Setup Suite 
    ${test_lib} =       Get Library Instance      crash_dump_etoe_report_test_lib
    ${test_result} =    Call Method               ${test_lib}    setup

Teardown Suite 
    ${test_lib} =       Get Library Instance      crash_dump_etoe_report_test_lib
    ${test_result} =    Call Method               ${test_lib}    teardown

*** Test Cases ***
Test Case - 1. Run crash dump validation
   # Call the test method from the test library and store the result
    ${test_lib} =       Get Library Instance      crash_dump_etoe_report_test_lib
    ${test_result} =    Call Method               ${test_lib}    run_crash_dump_etoe_report_test

   # Log the status
    Should be True    ${test_result}