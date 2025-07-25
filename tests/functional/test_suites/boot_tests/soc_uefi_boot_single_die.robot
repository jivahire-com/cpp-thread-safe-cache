# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies single die UEFI boot 

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.boot_tests.soc_uefi_boot
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/hio-211-j10-r5.json
...         WITH NAME    soc_uefi_boot_test_lib

*** Test Cases ***
Test Case - MSCP: SOC UEFI boot Test
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    soc_uefi_boot_test_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    soc_uefi_boot

    # Log the status
    Should be True    ${test_result}
