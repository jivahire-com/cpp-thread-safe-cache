# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies SOC_Mission_Mode_Disable test CLI commands.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.boot_tests.soc_mission_mode_disable
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    soc_mission_mode_disable_lib

*** Test Cases ***
Test Case - SOC_Mission_Mode_Disable
    # Get an instance of the SOC_Mission_Mode_Disable library
    ${test_lib} =      Get Library Instance    soc_mission_mode_disable_lib

    # Call the test method from the test library and store the result
    ${test_result} =    Call Method     ${test_lib}     soc_mission_mode_disable

    # Log the status
    Should Be True      ${test_result}
