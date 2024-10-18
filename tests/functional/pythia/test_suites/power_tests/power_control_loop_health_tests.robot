# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies Power loop Die0

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.power_tests.power_control_loop_health_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw.json
...         WITH NAME    power_control_loop_health_test_lib

*** Test Cases ***
Test Case - power control loop die0 test.
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    power_control_loop_health_test_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    power_control_loop_health_test

    # Log the status
    Should be True    ${test_result}
