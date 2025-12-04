# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies Power PLDM Service functionality including power cap effectors and throttle state sensors
Suite Setup      Setup Suite
Suite Teardown   Teardown Suite

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.power_tests.mod_power_pldm_service
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         dut_platform="KingsgateSOC"
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    mod_power_pldm_service_lib

*** Variables ***
${Success}    ${True}
${Failure}    ${False}

*** Keywords ***
Setup Suite 
    ${test_lib} =       Get Library Instance                 mod_power_pldm_service_lib
    ${test_result} =    Run Keyword And Return Status        Call Method    ${test_lib}    setup
    Run Keyword If      not ${test_result}    Fail           Suite setup failed. Aborting all tests.

Teardown Suite 
    ${test_lib} =       Get Library Instance                 mod_power_pldm_service_lib
    ${test_result} =    Run Keyword And Return Status        Call Method    ${test_lib}    teardown
    Run Keyword If      not ${test_result}    Fail           Suite teardown failed.

*** Test Cases ***
Test Case - 1. Power PLDM Prerequisites
    [Documentation]    Verify MCTP enumeration and PLDM sensor/effector registration
    ${result}=  mod_power_pldm_service_lib.prerequisite_get_mctp_id
    BuiltIn.Should Be Equal   ${result}   ${Success}

    ${result}=  mod_power_pldm_service_lib.prerequisite_verify_sensor_effector_registration
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 2. Power Throttle State Sensor Test
    [Documentation]    Test MCP throttle state sensor functionality by inducing power loop failures
    ${result}=  mod_power_pldm_service_lib.test_mcp_throttle_state_sensor
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 3. Power Cap Effector Test
    [Documentation]    Test MCP power cap effector functionality via PLDM commands
    ${result}=  mod_power_pldm_service_lib.test_mcp_power_cap_effector
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 4. Power Throttle Induction Test
    [Documentation]    Test end-to-end power throttling via BMC power capping
    ${result}=  mod_power_pldm_service_lib.test_induce_pwr_throttle_via_bmc_pwr_capping
    BuiltIn.Should Be Equal   ${result}   ${Success}
