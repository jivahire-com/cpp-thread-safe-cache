# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies SDM/CDED Heart Beat over UART

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.heart_beat.fbc_sdm_cded_heart_beat_single_die
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw.json
...         WITH NAME    fbc_sdm_cded_heart_beat_single_die_lib

*** Test Cases ***
Test Case - AP: SDM/CDED Full Boot Chain: [1140820]
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    fbc_sdm_cded_heart_beat_single_die_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    fbc_sdm_cded_heart_beat_single_die

    # Log the status
    Should be True    ${test_result}
