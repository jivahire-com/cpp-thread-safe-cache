# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies single die Windows boot 

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.boot_tests.soc_win_boot_dd
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
#...         host_config=${HOST_CONFIG_DIR}/debugpc-1103e12-n1.json
#...         host_config=${HOST_CONFIG_DIR}/C41431157B0204A.json
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    soc_win_boot_dd_test_lib

*** Test Cases ***
Test Case - MSCP: SOC Windows boot DualDie Test
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    soc_win_boot_dd_test_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    soc_win_boot_dd

    # Log the status
    Should be True    ${test_result}
