# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies SCP/MCP Heart Beat transmits on the UART

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../../pylibs
Library     pylibs.heart_beat.mscp_heart_beat_fpga
...             workspace_config=${WORKSPACE_CONFIG}
...             default_log_home=${LOG_DIR}
...             fw_payload_path=${PAYLOAD_DIR}
...             host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...             WITH NAME    heart_beat_test_lib

#This tag can be used for filtering, executing, or reporting specific tests related to this identifier.
Force Tags          1958704

*** Variables ***
${ado_number}       1958704

*** Test Cases ***
Test Case - MSCP: MSCP HeartBeat Test: FR [1423800] ${ado_number}
    # Get an instance of the test library
    ${test_object}=    Get Library Instance    heart_beat_test_lib
    
    # Call the test method from the test library and store the result
    ${test_pass}=    Call Method    ${test_object}    test

    # Log the status
    Should be True    ${test_pass}
