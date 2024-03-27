*** Settings ***
Documentation    Verifies SCP/MCP Heart Beat transmits on the UART

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ../../tests/heart_beat/heart_beat.py
...             workspace_config=${WORKSPACE_CONFIG}
...             default_log_home=${LOG_DIR}
...             fw_payload_path=${PAYLOAD_DIR}
...             host_config=${HOST_CONFIG}
...             WITH NAME    heart_beat_test_lib

*** Test Cases ***
Test Case - MSCP Heart Beat
    # Get an instance of the test libray
    ${test_object}=    Get Library Instance    heart_beat_test_lib
    
    # Call the test method from the test library and store the result
    ${test_pass}=    Call Method    ${test_object}    test

    # Log the status
    Should be True    ${test_pass}
