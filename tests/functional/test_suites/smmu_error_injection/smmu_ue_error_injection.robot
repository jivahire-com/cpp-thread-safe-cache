# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    SMMU UE (Uncorrectable Error) Injection Test
...              This test injects a TCU RPSS 0 TMU HTTU RAM uncorrectable error
...              and verifies the system triggers a Bug Check.

Library     ${CURDIR}/../../library
Library     library.smmu_ras_error_injection_tests.smmu_ue_error_injection
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    smmu_ue_error_injection_lib

*** Test Cases ***
Test Case - SMMU UE Error Injection
    [Documentation]    Injects SMMU TCU uncorrectable error and verifies Bug Check is triggered.
    [Tags]    smmu    ras    error_injection    uncorrectable_error

    # Get an instance of the SMMU UE error injection library
    ${test_lib} =      Get Library Instance    smmu_ue_error_injection_lib

    # Call the test method from the test library and store the result
    ${test_result} =    Call Method     ${test_lib}     smmu_ue_error_injection

    # Log the status
    Should Be True      ${test_result}
