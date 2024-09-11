# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Verifies CDED Pipeline Initialization.

# Import the python library, class must match filename when filepaths of full files.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.cded_tests.cded_pipeline_init_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/cded_pipeline_fw.json
...         WITH NAME    cded_pipeline_init_test_lib


*** Variables ***
@{REQUIRED_LOGS}        Setting up CDED opcode pipeline
...                     PDT PIPE0 ENABLED OPCODE: 9
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x1 NUM: 1
...                     CMP MASK: 0x1 NUM: 1
...                     PDT PIPE1 ENABLED OPCODE: 9
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x2 NUM: 1
...                     CMP MASK: 0x2 NUM: 1
...                     PDT PIPE2 ENABLED OPCODE: 6
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x4 NUM: 1
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE3 ENABLED OPCODE: 6
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x8 NUM: 1
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE4 ENABLED OPCODE: 6
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x10 NUM: 1
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE5 ENABLED OPCODE: 6
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x20 NUM: 1
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE6 ENABLED OPCODE: 6
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x40 NUM: 1
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE7 ENABLED OPCODE: 6
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x80 NUM: 1
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE8 ENABLED OPCODE: 32
...                     XTS MASK: 0x3 NUM: 2
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x0 NUM: 0
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE9 ENABLED OPCODE: 32
...                     XTS MASK: 0xc NUM: 2
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x0 NUM: 0
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE10 ENABLED OPCODE: 32
...                     XTS MASK: 0x30 NUM: 2
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x0 NUM: 0
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE11 ENABLED OPCODE: 32
...                     XTS MASK: 0xc0 NUM: 2
...                     GCM MASK: 0x0 NUM: 0
...                     DCM MASK: 0x0 NUM: 0
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE12 ENABLED OPCODE: 33
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x3 NUM: 2
...                     DCM MASK: 0x0 NUM: 0
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE13 ENABLED OPCODE: 33
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0xc NUM: 2
...                     DCM MASK: 0x0 NUM: 0
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE14 ENABLED OPCODE: 33
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0x30 NUM: 2
...                     DCM MASK: 0x0 NUM: 0
...                     CMP MASK: 0x0 NUM: 0
...                     PDT PIPE15 ENABLED OPCODE: 33
...                     XTS MASK: 0x0 NUM: 0
...                     GCM MASK: 0xc0 NUM: 2
...                     DCM MASK: 0x0 NUM: 0
...                     CMP MASK: 0x0 NUM: 0

*** Test Cases ***
Test Case - CDED: CDED Pipeline Init Test: [1140820]
    # Get an instance of the test library
    ${test_lib} =    Get Library Instance    cded_pipeline_init_test_lib
    
    # Call the test method from the test library and store the result
    ${test_result} =    Call Method    ${test_lib}    cded_pipeline_init_test       ${REQUIRED_LOGS}

    # Log the status
    Should be True    ${test_result}
