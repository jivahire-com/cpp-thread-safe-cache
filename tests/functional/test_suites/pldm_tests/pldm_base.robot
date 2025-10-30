# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies PLDM (Platform Level Data Management) functionality on the KingsgateSVP platform.
Suite Setup      Setup Suite
Suite Teardown   Teardown Suite


# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.pldm_tests.pldm_base
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         dut_platform="KingsgateSOC"
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    pldm_base_lib

*** Variables ***
${Success}    ${True}
${Failure}    ${False}

*** Keywords *** 
Setup Suite 
    ${test_lib} =       Get Library Instance      pldm_base_lib
    ${test_result} =    Call Method               ${test_lib}    setup

Teardown Suite 
    ${test_lib} =       Get Library Instance      pldm_base_lib
    ${test_result} =    Call Method               ${test_lib}    teardown

*** Test Cases ***
Test Case - 1. Connect to the MCP (Management Controller Processor)
    ${result}=  pldm_base_lib.testcase1_pldm_get_mctp_id
    Should Be Equal   ${result}   ${Success}

Test Case - 2.1. Get PLDM Terminus ID
    ${result}=  pldm_base_lib.testcase2_get_pldm_tid
    Should Be Equal   ${result}   ${Success}

Test Case - 2.2. Set PLDM Terminus ID
    ${result}=  pldm_base_lib.testcase2_set_pldm_tid
    Should Be Equal   ${result}   ${Success}

Test Case - 3. Verify PLDM Types
    ${result}=  pldm_base_lib.testcase3_verify_pldm_types
    Should Be Equal   ${result}   ${Success}

Test Case - 4.1. Verify PLDM Versions
    ${result}=  pldm_base_lib.testcase4_verify_pldm_versions
    Should Be Equal   ${result}   ${Success}

Test Case - 4.2. Verify PLDM Version Error
    ${result}=  pldm_base_lib.testcase4_verify_pldm_version_error
    Should Be Equal   ${result}   ${Success}   

Test Case - 5.1. Verify PLDM Commands
    ${result}=  pldm_base_lib.testcase5_verify_pldm_commands
    Should Be Equal   ${result}   ${Success}

Test Case - 5.2. Verify PLDM Commands Error
    ${result}=  pldm_base_lib.testcase5_verify_pldm_commands_error
    Should Be Equal   ${result}   ${Success}

Test Case - 6.1. Verify PLDM PDR Repository Info
    ${result}=  pldm_base_lib.testcase6_verify_get_pdr_repository_info
    Should Be Equal   ${result}   ${Success}

Test Case - 6.2. Verify PLDM PDR
    ${result}=  pldm_base_lib.testcase6_verify_get_pdr
    Should Be Equal   ${result}   ${Success}

Test Case - 6.3. Verify PLDM PDR Error
    ${result}=  pldm_base_lib.testcase6_verify_get_pdr_error
    Should Be Equal   ${result}   ${Success}