# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    SMMU CE (Correctable Error) Injection Test
...              This test injects a TCU RPSS 0 TMU HTTU RAM correctable error
...              and verifies the system handles it correctly.

Library     ${CURDIR}/../../library
Library     library.smmu_ras_error_injection_tests.smmu_ce_error_injection
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    smmu_ce_error_injection_lib

*** Test Cases ***
Test Case - SMMU CE Error Injection
    [Documentation]    Injects SMMU TCU correctable error and verifies system stability.
    [Tags]    smmu    ras    error_injection    correctable_error

    # Get an instance of the SMMU CE error injection library
    ${test_lib} =      Get Library Instance    smmu_ce_error_injection_lib

    # Call the test method from the test library and store the result
    ${test_result} =    Call Method     ${test_lib}     smmu_ce_error_injection

    # Log the status
    Should Be True      ${test_result}
