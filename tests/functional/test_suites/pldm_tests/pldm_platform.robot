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
Library     library.pldm_tests.pldm_platform
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         dut_platform="KingsgateSOC"
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    pldm_platform_lib

*** Variables ***
${Success}    ${True}
${Failure}    ${False}

*** Keywords *** 
Setup Suite 
    ${test_lib} =       Get Library Instance                 pldm_platform_lib
    ${test_result} =    Run Keyword And Return Status        Call Method    ${test_lib}    setup
    Run Keyword If      not ${test_result}    Log            Suite setup failed. Aborting all tests.
    Run Keyword If      not ${test_result}    Return From Keyword

Teardown Suite 
    ${test_lib} =       Get Library Instance                 pldm_platform_lib
    ${test_result} =    Run Keyword And Return Status        Call Method    ${test_lib}    teardown
    Run Keyword If      not ${test_result}    Log            Suite teardown failed.
    Run Keyword If      not ${test_result}    Return From Keyword

*** Test Cases ***
# Test case disabled due to bug: https://azurecsi.visualstudio.com/Dev/_workitems/edit/3191148,
# this will be re-enabled once bug is fixed
# Test Case - 1. Send a small Common Platform Error Record (CPER) PE message
#     ${result}=    pldm_platform_lib.testcase1_pldm_send_pe_cper_small
#     BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 2. Send a medium OEM Pre-Platform Event (PPE) message
    ${result}=    pldm_platform_lib.testcase2_pldm_send_ppe_oem_medium
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 3. Send a medium CPER Pre-Platform Event (PPE) message
    ${result}=    pldm_platform_lib.testcase3_pldm_send_ppe_cper_medium
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 4. Send a large OEM Pre-Platform Event (PPE) message
    ${result}=    pldm_platform_lib.testcase4_pldm_send_ppe_oem_large
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 5. Send a large CPER Pre-Platform Event (PPE) message
    ${result}=    pldm_platform_lib.testcase5_pldm_send_ppe_cper_large
    BuiltIn.Should Be Equal   ${result}   ${Success}
