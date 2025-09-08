# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies AVS Die1 reads

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.avs_tests.scp_avs_read_test_die1
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/debugpc-1103e12-n1.json
...         WITH NAME    avs_read_test_die1_lib

*** Test Cases ***
Test Case - SCP: SCP AVS Die1 read test.
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    avs_read_test_die1_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    avs_read_test_die1

    # Log the status
    Should be True    ${test_result}
