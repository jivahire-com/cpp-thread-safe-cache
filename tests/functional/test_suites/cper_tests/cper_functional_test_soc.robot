# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    CPER Functional E2E Test for SOC (dual die).

Library     ${CURDIR}/../../library
Library     library.cper_functional_soc_tests.cper_functional_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    cper_functional_test_lib

*** Test Cases ***
Test Case - CPER Functional E2E SOC
    ${test_lib}=       Get Library Instance    cper_functional_test_lib
    ${test_result}=    Call Method    ${test_lib}    cper_functional_test
    Should Be True     ${test_result}

