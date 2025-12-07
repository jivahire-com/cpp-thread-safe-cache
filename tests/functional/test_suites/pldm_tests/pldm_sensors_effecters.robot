# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Verifies the gpio driver commands from AP core 0.
Suite Setup      Setup Suite
Suite Teardown   Teardown Suite

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../library
Library     library.pldm_tests.pldm_sensors_effecters
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         dut_platform="KingsgateSOC"
...         host_config=${HOST_CONFIG_DIR}/${HOST_FILE_NAME}
...         WITH NAME    pldm_sensors_effecters_lib

*** Variables ***
${Success}    ${True}
${Failure}    ${False}

*** Keywords ***
Setup Suite 
    ${test_lib} =       Get Library Instance                 pldm_sensors_effecters_lib
    ${test_result} =    Run Keyword And Return Status        Call Method    ${test_lib}    setup
    Run Keyword If      not ${test_result}    Log            Suite setup failed. Aborting all tests.
    Run Keyword If      not ${test_result}    Return From Keyword

Teardown Suite 
    ${test_lib} =       Get Library Instance                 pldm_sensors_effecters_lib
    ${test_result} =    Run Keyword And Return Status        Call Method    ${test_lib}    teardown
    Run Keyword If      not ${test_result}    Log            Suite teardown failed.
    Run Keyword If      not ${test_result}    Return From Keyword

*** Test Cases ***
Test Case - 1. Set Numeric Sensor Enable
    ${result}=  pldm_sensors_effecters_lib.testcase_01_set_numeric_sensor_enable
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 2. Set Numeric Sensor Enable (Error Case)
    ${result}=  pldm_sensors_effecters_lib.testcase_02_set_numeric_sensor_enable_error_case
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 3. Get Numeric Sensor Reading
    ${result}=  pldm_sensors_effecters_lib.testcase_03_get_numeric_sensor_reading
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 4. Get Numeric Sensor Reading (Error Case)
    ${result}=  pldm_sensors_effecters_lib.testcase_04_get_numeric_sensor_reading_error_case
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 5. Set State Sensor Enable
    ${result}=  pldm_sensors_effecters_lib.testcase_05_set_state_sensor_enables
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 6. Set State Sensor Enable (Error Case)
    ${result}=  pldm_sensors_effecters_lib.testcase_06_set_state_sensor_enables_error_cases
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 7. Get State Sensor Reading
    ${result}=  pldm_sensors_effecters_lib.testcase_07_get_state_sensor_reading
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 8. Get State Sensor Reading (Error Case)
    ${result}=  pldm_sensors_effecters_lib.testcase_08_get_state_sensor_reading_error_case
    BuiltIn.Should Be Equal   ${result}   ${Success}


Test Case - 9. Set Numeric Effecter
    ${result}=  pldm_sensors_effecters_lib.testcase_09_set_numeric_effecter
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 10. Set Numeric Effecter (Error Case)
    ${result}=  pldm_sensors_effecters_lib.testcase_10_set_numeric_effecter_error_case
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 11. Get Numeric Effecter
    ${result}=  pldm_sensors_effecters_lib.testcase_11_get_numeric_effecter
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 12. Get Numeric Effecter (Error Case)
    ${result}=  pldm_sensors_effecters_lib.testcase_12_get_numeric_effecter_error_case
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 13. Set State Effecter States
    ${result}=  pldm_sensors_effecters_lib.testcase_13_set_state_effecter_states
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 14. Set State Effecter States (Error Case)
    ${result}=  pldm_sensors_effecters_lib.testcase_14_set_state_effecter_states_error_case
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 15. Get State Effecter States
    ${result}=  pldm_sensors_effecters_lib.testcase_15_get_state_effecter_states
    BuiltIn.Should Be Equal   ${result}   ${Success}

Test Case - 16. Get State Effecter States (Error Case)
    ${result}=  pldm_sensors_effecters_lib.testcase_16_get_state_effecter_states_error_case
    BuiltIn.Should Be Equal   ${result}   ${Success}
