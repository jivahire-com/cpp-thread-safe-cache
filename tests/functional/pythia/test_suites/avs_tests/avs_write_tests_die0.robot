# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies AVS writes

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.avs_tests.scp_avs_write_test_die0
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw.json
...         WITH NAME    avs_write_test_die0_lib

*** Test Cases ***
Test Case - SCP: SCP AVS write test.
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    avs_write_test_die0_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    avs_write_test_die0

    # Log the status
    Should be True    ${test_result}
