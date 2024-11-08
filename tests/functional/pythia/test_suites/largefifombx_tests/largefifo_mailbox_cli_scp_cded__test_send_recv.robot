# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies Large FIFO Mailbox CLI SCP CDED send recv command.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.largefifombx_tests.largefifo_mailbox_cli_scp_cded_test_send_recv
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    largefifo_mailbox_cli_scp_cded_test_send_recv_lib

*** Test Cases ***
Test Case - Large FIFO Mailbox CLI Send Recv Test
    [Documentation]     Verifies Large FIFO Mailbox CLI SCP CDED Send Recv command.

    # Get an instance of the largefifo_mailbox_cli_test_lib library
    ${test_lib} =       Get Library Instance      largefifo_mailbox_cli_scp_cded_test_send_recv_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         largefifo_mailbox_cli_scp_cded_test_send_recv

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}