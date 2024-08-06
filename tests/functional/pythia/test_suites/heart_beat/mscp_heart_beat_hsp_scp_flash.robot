*** Settings ***
Documentation    Verifies SCP Bootloader FW (with mailbox enabled) Heart Beat transmits on the UART

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../tests
Library     tests.heart_beat.mscp_heart_beat_bootloader
...             workspace_config=${WORKSPACE_CONFIG}
...             default_log_home=${LOG_DIR}
...             fw_payload_path=${PAYLOAD_DIR}
...             host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw.json
...             test_core=hsp_scp
...             WITH NAME    heart_beat_test_lib

*** Test Cases ***
Test Case - SCP HSP BL Heart Beat MBOX
    # Get an instance of the test library
    ${test_object}=    Get Library Instance    heart_beat_test_lib
    
    # Call the test method from the test library and store the result
    ${test_pass}=    Call Method    ${test_object}    test

    # Log the status
    Should be True    ${test_pass}
