# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies the sdm memcpy completes from AP core 0.

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../../pylibs/sdm_pcie_tests/sdm_pcie_tests.py
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/sdm_pcie_tests.json
...         WITH NAME    sdm_pcie_tests_lib

#This tag can be used for filtering, executing, or reporting specific tests related to this identifier.
Force Tags          1958829

*** Variables ***
@{FAILURE_STRINGS}    FAIL
@{SUCCESS_STRINGS}    wait_for_cmd_completion-: INFO: RX: SUCCESS
...                   compare_data_buffer: INFO: SUCCESS
...                   sdm_test-: ****  SDM Test DONE ****
${ado_number}       1958829

*** Test Cases ***
Test Case - AP: SDM memcpy Test: FR [1423800] ${ado_number}
    # Get an instance of the test library
    ${test_lib}=    Get Library Instance    sdm_pcie_tests_lib
    
    # Call the test method from the test library and store the results
    ${test_setup}=    Call Method    ${test_lib}    test_setup

    ${dut_connection}=     Set Variable        ${test_lib.dut.mb.node_0.soc.primary_die.apns.channel_manager}
    Set Suite Variable  ${apns_series}   ${dut_connection}

    ${cmd_results}=    write to uart  ${apns_series}  ap_bm sdm_test
    Log            ${cmd_results}

    ${results}=        read from uart  ${apns_series}  ${35}
    Log            ${results}

    ${response}=    Call Method    ${test_lib}    strings_to_look    ${results}    ${SUCCESS_STRINGS}    ${FAILURE_STRINGS}
    Log                   ${response}

    # Validate the presence of success strings and absence of failure strings
    Should Be True    ${response}

    # Perform cleanup by calling the test_teardown method
    Call Method    ${test_lib}    test_teardown