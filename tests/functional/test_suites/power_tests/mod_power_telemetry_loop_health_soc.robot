# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies that power telemetry loop health status reports no errors

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.power_tests.mod_power_telemetry_loop_health
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    mod_power_telemetry_loop_health_lib

*** Test Cases ***
Test Case - SCP: Power telemetry loop health test
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    mod_power_telemetry_loop_health_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    power_telemetry_loop_health_test

    # Log the status
    Should be True    ${test_result}