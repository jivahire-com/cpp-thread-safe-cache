# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies the gpio driver commands from AP core 0.

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.gpio_driver_tests.gpio_driver_test
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw.json
...         gpio_config_path=${CURDIR}/gpio_test_config.json
...         WITH NAME    gpio_driver_test_lib

*** Variables ***

*** Test Cases ***
Test Case - MSCP: GPIO Driver Test: FR [1978750]
    [Documentation]     Test the GPIO Driver from MSCP.
    
    # Get an instance of the gpio_driver_test_lib library
    ${test_lib} =       Get Library Instance      gpio_driver_test_lib

    # Check for both failure and success strings in the output.
    ${test_result} =        Call Method         ${test_lib}         test_gpio_driver

    # Validate that parse_log returned test PASS or FAIL
    Should Be True      ${test_result}