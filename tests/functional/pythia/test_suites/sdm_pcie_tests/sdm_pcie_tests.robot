*** Settings ***
Documentation    Verifies the sdm memcpy completes from AP core 0.

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../tests/sdm_pcie_tests/sdm_pcie_tests.py
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/sdm_pcie_tests.json
...         WITH NAME    sdm_pcie_tests_lib

*** Variables ***
@{SUCCESS_STRINGS}    wait_for_cmd_completion-250: INFO: RX: SUCCESS
...                   compare_data_buffer: INFO: SUCCESS
...                   sdm_test-474: ****  SDM Test DONE ****

*** Test Cases ***
Test Case - SDM memcpy test
    # Get an instance of the test library
    ${test_object}=    Get Library Instance    sdm_pcie_tests_lib
    
    # Call the test method from the test library and store the result
    ${test_pass}=    Call Method    ${test_object}    test_setup

    ${dut}=     Set Variable        ${test_object.dut.mb.node_0.soc.primary_die.apns.channel_manager}
    Set Suite Variable  ${dut_connection}   ${dut}

    ${cmd_res}=    write to uart  ${dut_connection}  ap_bm sdm_test
    Log            ${cmd_res}

    ${res}=        read from uart  ${dut_connection}  ${34}
    Log            ${res}

    ${found_strings}=    Call Method    ${test_object}    check_success_strings    ${res}    @{SUCCESS_STRINGS}
    Log                   ${found_strings}

    Should Not Be Empty   ${found_strings}
