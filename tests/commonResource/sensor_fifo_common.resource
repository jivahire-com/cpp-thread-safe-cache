*** Settings ***
Library           ${CURDIR}/../functional/pythia/pylibs
Library           pylibs.sensor_fifo_tests.sensor_fifo_cli_test.SensorFifoCliTest
...               workspace_config=${WORKSPACE_CONFIG}
...               default_log_home=${LOG_DIR}
...               fw_payload_path=${PAYLOAD_DIR}
...               host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...               WITH NAME    sensor_fifo_cli_test_lib

*** Keywords ***
Run Sensor Fifo CLI Test
    [Arguments]    ${command}    ${read_until_key_string}    ${pass_response_list}
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${test_result}=    Run Keyword And Continue On Failure    
    ...    Call Method    ${test_lib}    sensor_fifo_cli_test    ${command}    ${read_until_key_string}    ${pass_response_list}
    Should Be True    ${test_result}

Run Sensor Fifo CLI Test with optional first command
    [Arguments]    ${command}    ${read_until_key_string}    ${pass_response_list}    ${optional_first_command} 
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${test_result}=    Run Keyword And Continue On Failure    
    ...    Call Method    ${test_lib}    sensor_fifo_cli_test    ${command}    ${read_until_key_string}    ${pass_response_list}    ${optional_first_command}
    Should Be True    ${test_result}
