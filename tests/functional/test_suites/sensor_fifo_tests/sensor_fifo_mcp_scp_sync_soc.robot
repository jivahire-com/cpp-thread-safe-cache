# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies that the sensor fifos are synchronized between the MCP and the SCP on SoC

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.sensor_fifo_tests.sensor_fifo_sync_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/debugpc-1103e12-n1.json
...         WITH NAME    sensor_fifo_sync_test_lib

*** Test Cases ***
Test Case - SCP: Sensor FIFO Synchronize Die 0
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    sensor_fifo_sync_test_lib
    
    # Call the test method from the test library and store the result
    # Call it with Die 0
    ${test_result} =    Call Method    ${test_lib}    sync_test    0

    # Log the status
    Should be True    ${test_result}

Test Case - SCP: Sensor FIFO Synchronize Die 1
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    sensor_fifo_sync_test_lib
    
    # Call the test method from the test library and store the result
    # Call it with Die 1
    ${test_result} =    Call Method    ${test_lib}    sync_test    1

    # Log the status
    Should be True    ${test_result}
