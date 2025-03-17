# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies the enumeration of SDM and CDED cores from AP core 0.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.kng_pythia_libs.APBaremetalTests
...         name=Sdm_Pcie_Tests_log
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/single_die_flash_boot.json
...         WITH NAME    APBaremetalTestsLib


*** Test Cases ***
Test Case - SVP: SVP TearDown.
    
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    APBaremetalTestsLib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    test_teardown_force

    # Log the status
    Should be True    ${test_result}