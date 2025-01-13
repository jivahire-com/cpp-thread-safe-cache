# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies MSCP Heart Beat over UART

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.heart_beat.mscp_heart_beat_dual_die
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw_dual_die.json
...         WITH NAME    mscp_heartbeat_test_lib

*** Test Cases ***
Test Case - MSCP: MSCP HeartBeat Test: FR [1423800]
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    mscp_heartbeat_test_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    mscp_heartbeat

    # Log the status
    Should be True    ${test_result}
