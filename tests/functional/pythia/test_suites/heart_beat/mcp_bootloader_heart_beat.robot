# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies MCP Bootloader FW Heart Beat on UART

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.heart_beat.mcp_heart_beat
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mcp_bl_embed_fw.json
...         WITH NAME    mcp_heart_beat_lib

*** Test Cases ***
Test Case - MCP: MCP Bootloader HeartBeat Test: FR [1423800]
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    mcp_heart_beat_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    test_mcp_bl_embed_fw

    # Log the status
    Should be True    ${test_result}
