# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies MSCP DDR Boot Dual over UART

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.ddrboot_tests.mscp_ddrboot_dual_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw_dual_die.json
...         WITH NAME    mscp_ddrboot_test_lib

*** Test Cases ***
Test Case - MSCP: MSCP DDR Boot Dual Test: 
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    mscp_ddrboot_test_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    mscp_ddrboot_test

    # Log the status
    Should be True    ${test_result}
