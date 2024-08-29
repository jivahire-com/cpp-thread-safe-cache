# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies SCP Heart Beat over UART

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.heart_beat.scp_heart_beat
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    scp_heartbeat_test_lib

*** Test Cases ***
Test Case - SCP: SCP HeartBeat Test: FR [1423800]
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    scp_heartbeat_test_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    scp_heartbeat

    # Log the status
    Should be True    ${test_result}
