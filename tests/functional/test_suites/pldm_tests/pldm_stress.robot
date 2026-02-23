# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies PLDM platform functionality.
Suite Setup      Setup Suite
Suite Teardown   Teardown Suite

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.pldm_tests.pldm_stress
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         dut_platform="KingsgateSVP"
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    pldm_stress_lib

*** Variables ***
${Success}    ${True}
${Failure}    ${False}

*** Keywords ***
Setup Suite 
    ${test_lib} =       Get Library Instance                 pldm_stress_lib
    ${test_result} =    Run Keyword And Return Status        Call Method    ${test_lib}    setup
    Run Keyword If      not ${test_result}    Log            Suite setup failed. Aborting all tests.
    Run Keyword If      not ${test_result}    Return From Keyword

Teardown Suite 
    ${test_lib} =       Get Library Instance                 pldm_stress_lib
    ${test_result} =    Run Keyword And Return Status        Call Method    ${test_lib}    teardown
    Run Keyword If      not ${test_result}    Log            Suite teardown failed.
    Run Keyword If      not ${test_result}    Return From Keyword

*** Test Cases ***
# The argument is the duration of the stress test in minutes.
Test Case - Stress Test PLDM Platform and Sensors Effecters Functionality
    ${result}=    pldm_stress_lib.testcase_pldm_stress_test    30
    BuiltIn.Should Be Equal   ${result}   ${Success}
