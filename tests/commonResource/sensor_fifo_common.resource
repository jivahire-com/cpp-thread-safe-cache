*** Settings ***
Library           ${CURDIR}/../functional/pythia/pylibs
Library           pylibs.sensor_fifo_tests.sensor_fifo_cli_test.SensorFifoCliTest
...               workspace_config=${WORKSPACE_CONFIG}
...               default_log_home=${LOG_DIR}
...               fw_payload_path=${PAYLOAD_DIR}
...               host_config=${HOST_CONFIG_DIR}/hsp_scp_bl_embed_fw.json
...               WITH NAME    sensor_fifo_cli_test_lib

Library           ${CURDIR}/../functional/pythia/pylibs/sensor_fifo_tests/sensor_fifo_lib.py
Library           ${CURDIR}/../functional/pythia/pylibs/sensor_fifo_tests/sensor_data_generator.py

*** Keywords ***
Validate Test Result
    [Arguments]    ${test_result}
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    Call Method    ${test_lib}    validate_test_result    ${test_result}

Run Sensor Fifo CLI Test
    [Arguments]    ${command}    ${read_until_key_string}    ${pass_response_list}
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${test_result}=    Run Keyword And Continue On Failure    
    ...    Call Method    ${test_lib}    sensor_fifo_cli_test    ${command}    ${read_until_key_string}    ${pass_response_list}
    Validate Test Result    ${test_result}

Run Sensor Fifo CLI Test with optional first command
    [Arguments]    ${command}    ${read_until_key_string}    ${pass_response_list}    ${optional_first_command} 
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${test_result}=    Run Keyword And Continue On Failure    
    ...    Call Method    ${test_lib}    sensor_fifo_cli_test    ${command}    ${read_until_key_string}    ${pass_response_list}    ${optional_first_command}
    Validate Test Result    ${test_result}

Get FIFO Information
    [Arguments]    ${fifo_id}=${None}
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${fifo_info}=    Call Method    ${test_lib}    get_structured_fifo_info    ${fifo_id}
    RETURN    ${fifo_info}

Set And Verify FIFO State
    [Arguments]    ${fifo_id}    ${enable}
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${result}=    Call Method    ${test_lib}    set_and_verify_fifo_state    ${fifo_id}    ${enable}
    Should Be True    ${result}    Failed to set and verify FIFO ${fifo_id} state to ${enable}

Setup Test Environment
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${setup_result}=    Call Method    ${test_lib}    setup_dut
    Should Be True    ${setup_result}    Failed to setup DUT

Teardown Test Environment
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${teardown_result}=    Call Method    ${test_lib}    teardown_dut
    Should Be True    ${teardown_result}    Failed to teardown DUT

Run Command On SCP
    [Arguments]    ${command}    ${read_until_key}
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${result}=    Call Method    ${test_lib}    run_command_on_scp    ${command}    ${read_until_key}
    [Return]    ${result}

Enable FIFO
    [Arguments]    ${fifo_id}
    Set And Verify FIFO State    ${fifo_id}    ${True}

Disable FIFO
    [Arguments]    ${fifo_id}
    Set And Verify FIFO State    ${fifo_id}    ${False}

Get All FIFO IDs
    [Documentation]    Returns a sorted list of all available FIFO IDs
    ${all_fifos}=    Get FIFO Information
    ${fifo_ids}=    Evaluate    sorted(${all_fifos}.keys())
    [Return]    ${fifo_ids}

Get FIFO Name
    [Arguments]    ${fifo_id}
    ${info}=    Get FIFO Information    ${fifo_id}
    [Return]    ${info}[name]

Verify FIFO State
    [Arguments]    ${fifo_id}    ${expected_state}
    ${info}=    Get FIFO Information    ${fifo_id}
    Should Be Equal    ${info}[enabled]    ${expected_state}    FIFO ${fifo_id} state mismatch

Is FIFO Empty
    [Arguments]    ${fifo_id}
    ${info}=    Get FIFO Information    ${fifo_id}
    [Return]    ${info}[is_empty]

Enable All Available FIFOs
    [Documentation]    Enables all available FIFOs in the system
    
    # Get all FIFO information
    ${all_fifos}=    Get FIFO Information
    
    # Sort FIFO IDs for consistent testing order
    ${fifo_ids}=    Evaluate    sorted(${all_fifos}.keys())
    
    # Iterate through each FIFO
    FOR    ${fifo_id}    IN    @{fifo_ids}
        ${fifo_name}=    Set Variable    ${all_fifos}[${fifo_id}][name]
        
        # Enable FIFO
        Enable FIFO    ${fifo_id}
        Verify FIFO State    ${fifo_id}    ${True}
    END

Disable All Available FIFOs
    [Documentation]    Disables all available FIFOs in the system
    
    # Get all FIFO information
    ${all_fifos}=    Get FIFO Information
    
    # Sort FIFO IDs for consistent testing order
    ${fifo_ids}=    Evaluate    sorted(${all_fifos}.keys())
    
    # Iterate through each FIFO
    FOR    ${fifo_id}    IN    @{fifo_ids}
        ${fifo_name}=    Set Variable    ${all_fifos}[${fifo_id}][name]
        
        # Disable fifo
        Disable FIFO    ${fifo_id}
        Verify FIFO State    ${fifo_id}    ${False}
    END