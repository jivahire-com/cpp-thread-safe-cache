# Copyright (c) Microsoft Corporation. All rights resultserved.

*** Settings ***
Documentation    Verifies the SDM E2I I2E completes from AP core 0.

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../../pylibs/sdm_pcie_tests/sdm_pcie_tests.py
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/sdm_pcie_tests.json
...         WITH NAME    sdm_pcie_tests_lib

#This tag can be used for filtering, executing, or reporting specific tests related to this identifier.
Force Tags          1958842

*** Variables ***
@{FAILURE_STRINGS}    FAIL
@{SUCCESS_STRINGS}    User Defined Loopback Test End!
${ado_number}       1958842

*** Test Cases ***
Test Case - AP: SDM E2I I2E Test: FR [1738499] ${ado_number}
    # Get an instance of the test library
    ${test_lib}=    Get Library Instance    sdm_pcie_tests_lib
    
    # Call the test method from the test library and store the results
    ${test_setup}=    Call Method    ${test_lib}    test_setup

    ${apns_conct}=     Set Variable        ${test_lib.dut.mb.node_0.soc.primary_die.apns.channel_manager}
    Set Suite Variable  ${apns_series}   ${apns_conct}

    ${cmd_results}=    write to uart  ${apns_series}  ap_bm user_def_loopback_test 0 0
    Log            ${cmd_results}

    ${results}=        read from uart  ${apns_series}  ${35}
    Log            ${results}

    ${response}=    Call Method    ${test_lib}    strings_to_look    ${results}    ${SUCCESS_STRINGS}    ${FAILURE_STRINGS}
    Log                   ${response}

    # Validate the presence of success strings and absence of failure strings
    Should Be True    ${response}

    # Perform cleanup by calling the test_teardown method
    Call Method    ${test_lib}    test_teardown