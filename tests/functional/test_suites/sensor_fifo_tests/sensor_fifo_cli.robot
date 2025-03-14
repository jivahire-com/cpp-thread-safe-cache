*** Settings ***
Documentation     Tests for sensor FIFO CLI commands
...               Verifies various sensor FIFO CLI commands functionality

Resource          ${CURDIR}/../../../commonResource/sensor_fifo_common.resource

Suite Setup       Run Keywords
...               Setup Test Environment    AND
...               Setup FIFO Prerequisites
Suite Teardown    Teardown Test Environment

*** Keywords ***
Setup FIFO Prerequisites
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${prereq_status}=    Call Method    ${test_lib}    setup_fifo_prerequisites
    IF    not ${prereq_status}    Fail    Failed to set up FIFO test prerequisites

*** Test Cases ***
Test Case - verify snsrfifo help command
    [Documentation]    Verifies the sensor FIFO CLI help command functionality
    [Tags]    ssi    ssi_sensor_fifo_cli    TEST_CASE_ID:2153985
    
    # Test-specific variables
    ${HELP_COMMAND}=    Set Variable    snsrfifo ?
    ${READ_UNTIL_KEY_STRING}=    Set Variable    Display menu or usage information
    @{HELP_RESPONSE_LIST}=    Create List
    ...    rdreg    Read a Register
    ...    wrreg    Write a Register
    ...    lprop    List Fifo Properties
    ...    globalhwen    Set Global HW Enable
    ...    reset    Reset all Fifos
    ...    fifoen    Enable/Disable a Fifo
    ...    rdentry    Read an entry from a fifo
    ...    wrentry    Write an entry to a fifo
    ...    updstride    Update stride index
    ...    ..    Goto Main Menu
    ...    ?    Display menu or usage information
    
    Run Sensor Fifo CLI Test    ${HELP_COMMAND}    ${READ_UNTIL_KEY_STRING}    ${HELP_RESPONSE_LIST}

Test Case - verify fifoen command
    [Documentation]    Verifies the sensor FIFO enable/disable functionality
    [Tags]    ssi    ssi_sensor_fifo_cli    TEST_CASE_ID:2133547
    
    # Test-specific variables
    @{FIFO_IDS}=    Create List    0    1    2    3    4    5    6    7    8    9
    
    # Get all FIFO information
    ${all_fifos}=    Get FIFO Information
    
    # Iterate through each FIFO
    FOR    ${fifo_id}    IN    @{FIFO_IDS}
        ${matching_fifo}=    Set Variable    ${None}
        FOR    ${index}    ${fifo_info}    IN    &{all_fifos}
            IF    ${fifo_info}[fifo_id] == ${fifo_id}
                ${matching_fifo}=    Set Variable    ${fifo_info}
                BREAK
            END
        END
        
        # Log failure for non-matching FIFO and continue
        IF    ${matching_fifo} == ${None}
            Run Keyword And Continue On Failure    Fail    FIFO ID ${fifo_id} not found in system
            Continue For Loop
        END
        
        ${fifo_name}=    Set Variable    ${matching_fifo}[name]
        Log    Testing FIFO ${fifo_id}: ${fifo_name}
        
        # Test disabling
        Disable FIFO    ${fifo_id}
        Verify FIFO State    ${fifo_id}    ${False}
        
        # Test re-enabling
        Enable FIFO    ${fifo_id}
        Verify FIFO State    ${fifo_id}    ${True}
    END

Test Case - verify snsrfifo lprop command
    [Documentation]    Verifies sensor FIFO lprop command functionality
    [Tags]    ssi    ssi_sensor_fifo_lprop    TEST_CASE_ID:2143251
    
    # Test-specific variables
    ${HELP_COMMAND}=    Set Variable    snsrfifo lprop
    ${READ_UNTIL_KEY_STRING}=    Set Variable    Ok
    @{HELP_RESPONSE_LIST}=    Create List
    ...    Fifo ID = 0: Fifo Name = PSTATE Fifo
    ...    Fifo ID = 1: Fifo Name = SCP Msg Fifo
    ...    Fifo ID = 2: Fifo Name = Tile Temperature Fifo
    ...    Fifo ID = 3: Fifo Name = Tile Voltage Fifo
    ...    Fifo ID = 4: Fifo Name = Core Current Fifo
    ...    Fifo ID = 5: Fifo Name = PVT Temperature Fifo
    ...    Fifo ID = 6: Fifo Name = PVT Voltage Fifo
    ...    Fifo ID = 7: Fifo Name = DIMM Fifo
    ...    Fifo ID = 8: Fifo Name = VR Temp Fifo
    ...    Fifo ID = 9: Fifo Name = VR Current Fifo
    ...    Ok
    
    Run Sensor Fifo CLI Test    ${HELP_COMMAND}    ${READ_UNTIL_KEY_STRING}    ${HELP_RESPONSE_LIST}

Test Case - verify globalhwen command
    [Documentation]    Verifies the sensor FIFO global hardware enable/disable functionality
    ...    
    ...    The test leverages the existing `Run Sensor Fifo CLI Test` keyword from the resource file, 
    ...    which internally uses the Python test class sensor_fifo_cli_test which is part of 
    ...    tests\functional\pythia\library\sensor_fifo_tests\sensor_fifo_cli_test.py
    ...    
    ...    Test Steps:
    ...    1. Verify help output for globalhwen command:
    ...       - Command: 'snsrfifo globalhwen ?'
    ...       - Expected: Shows usage 'Usage: globalhwen <0,1>'
    ...    
    ...    2. Test global hardware disable:
    ...       - Command: 'snsrfifo globalhwen 0'
    ...       - Expected: 'Global HW Enable set to disable'
    ...    
    ...    3. Test global hardware enable:
    ...       - Command: 'snsrfifo globalhwen 1'
    ...       - Expected: 'Global HW Enable set to enable'
    
    [Tags]    ssi    ssi_sensor_fifo_cli    TEST_CASE_ID:2143252
    
    # Test-specific variables
    ${DISABLE_COMMAND}=    Set Variable    snsrfifo globalhwen 0
    ${ENABLE_COMMAND}=    Set Variable    snsrfifo globalhwen 1
    ${HELP_COMMAND}=    Set Variable    snsrfifo globalhwen ?
    
    # Verify help output
    ${READ_UNTIL_KEY_STRING}=    Set Variable    Usage: globalhwen <0,1>
    @{HELP_RESPONSE_LIST}=    Create List    Usage: globalhwen <0,1>
    Run Sensor Fifo CLI Test    ${HELP_COMMAND}    ${READ_UNTIL_KEY_STRING}    ${HELP_RESPONSE_LIST}
    
    # Test disabling global hardware
    ${READ_UNTIL_KEY_STRING}=    Set Variable    Ok
    @{DISABLE_RESPONSE_LIST}=    Create List    Global HW Enable set to disable
    Run Sensor Fifo CLI Test    ${DISABLE_COMMAND}    ${READ_UNTIL_KEY_STRING}    ${DISABLE_RESPONSE_LIST}
    
    # Test enabling global hardware
    @{ENABLE_RESPONSE_LIST}=    Create List    Global HW Enable set to enable
    Run Sensor Fifo CLI Test    ${ENABLE_COMMAND}    ${READ_UNTIL_KEY_STRING}    ${ENABLE_RESPONSE_LIST}

Test Case - verify rdreg and wrreg command
    [Documentation]    Verifies the PSTATE FIFO enable/disable functionality using direct register access
    ...    
    ...    Uses register PSTATE_HW_EN_CTRL_REG_ADDR (0x1130104) which is equivalent to 
    ...    SCF_MHU_PSTATE_CONFIG_ADDRESS to control PSTATE FIFO state
    ...    
    ...    The test uses keywords from sensor_fifo_common.resource to verify FIFO state
    ...    
    ...    Test Steps:
    ...    1. Get initial FIFO states using 'snsrfifo lprop'
    ...    2. Disable PSTATE FIFO:
    ...       - Write 0x0 to register: 'snsrfifo wrreg 0x1130104 0x0'
    ...       - Read register value: 'snsrfifo rdreg 0x1130104'
    ...       - Verify FIFO state is disabled
    ...    3. Enable PSTATE FIFO:
    ...       - Write 0x1 to register: 'snsrfifo wrreg 0x1130104 0x1'
    ...       - Read register value: 'snsrfifo rdreg 0x1130104'
    ...       - Verify FIFO state is enabled
    ...    4. Final disable PSTATE FIFO:
    ...       - Write 0x0 to register: 'snsrfifo wrreg 0x1130104 0x0'
    ...       - Read register value: 'snsrfifo rdreg 0x1130104'
    ...       - Verify FIFO state is disabled
    ...    5. Get final FIFO states to verify overall status

    [Tags]    ssi    ssi_sensor_fifo_cli    TEST_CASE_ID:2139172
    
    # Test-specific variables
    ${PSTATE_HW_EN_CTRL_REG}=    Set Variable    0x1130104    # SCF_MHU_PSTATE_CONFIG_ADDRESS
    ${PSTATE_FIFO_ID}=    Set Variable    ${0}
    ${READ_UNTIL_KEY}=    Set Variable    Ok
    
    # Get initial FIFO state
    ${fifo_info}=    Get FIFO Information
    
    # First disable operation
    ${disable_cmd}=    Set Variable    snsrfifo wrreg ${PSTATE_HW_EN_CTRL_REG} 0x0
    @{disable_response}=    Create List    Write SCF Register: Address ${PSTATE_HW_EN_CTRL_REG} = 0x0
    Run Sensor Fifo CLI Test    ${disable_cmd}    ${READ_UNTIL_KEY}    ${disable_response}
    
    # Verify register value after disable
    ${read_cmd}=    Set Variable    snsrfifo rdreg ${PSTATE_HW_EN_CTRL_REG}
    @{read_response}=    Create List    Read SCF Register: Address ${PSTATE_HW_EN_CTRL_REG} = 0x0
    Run Sensor Fifo CLI Test    ${read_cmd}    ${READ_UNTIL_KEY}    ${read_response}
    
    # Verify PSTATE FIFO is disabled
    Verify FIFO State    ${PSTATE_FIFO_ID}    ${False}
    
    # Enable operation
    ${enable_cmd}=    Set Variable    snsrfifo wrreg ${PSTATE_HW_EN_CTRL_REG} 0x1
    @{enable_response}=    Create List    Write SCF Register: Address ${PSTATE_HW_EN_CTRL_REG} = 0x1
    Run Sensor Fifo CLI Test    ${enable_cmd}    ${READ_UNTIL_KEY}    ${enable_response}
    
    # Verify register value after enable
    ${read_cmd}=    Set Variable    snsrfifo rdreg ${PSTATE_HW_EN_CTRL_REG}
    @{read_response}=    Create List    Read SCF Register: Address ${PSTATE_HW_EN_CTRL_REG} = 0x1
    Run Sensor Fifo CLI Test    ${read_cmd}    ${READ_UNTIL_KEY}    ${read_response}
    
    # Verify PSTATE FIFO is enabled
    Verify FIFO State    ${PSTATE_FIFO_ID}    ${True}
    
    # Final disable operation
    ${disable_cmd}=    Set Variable    snsrfifo wrreg ${PSTATE_HW_EN_CTRL_REG} 0x0
    @{disable_response}=    Create List    Write SCF Register: Address ${PSTATE_HW_EN_CTRL_REG} = 0x0
    Run Sensor Fifo CLI Test    ${disable_cmd}    ${READ_UNTIL_KEY}    ${disable_response}
    
    # Verify register value after final disable
    ${read_cmd}=    Set Variable    snsrfifo rdreg ${PSTATE_HW_EN_CTRL_REG}
    @{read_response}=    Create List    Read SCF Register: Address ${PSTATE_HW_EN_CTRL_REG} = 0x0
    Run Sensor Fifo CLI Test    ${read_cmd}    ${READ_UNTIL_KEY}    ${read_response}
    
    # Verify PSTATE FIFO is disabled again
    Verify FIFO State    ${PSTATE_FIFO_ID}    ${False}
    
    # Get final FIFO information to show all FIFOs state
    ${final_fifo_info}=    Get FIFO Information


Test Case - verify fifo reset command
    [Documentation]    Verifies the sensor FIFO reset command functionality
    ...    
    ...    The test uses keywords from sensor_fifo_common.resource to verify FIFO states
    ...    
    ...    Test Steps:
    ...    1. Gets initial states for FIFOs 0-4 with single lprop command
    ...    2. Enables any disabled FIFOs in range 0-4
    ...    3. Executes reset command
    ...    4. Verifies all FIFOs 0-4 are disabled after reset

    [Tags]    ssi    ssi_sensor_fifo_cli    TEST_CASE_ID:2143253
    
    # Get initial FIFO states with single lprop command
    ${initial_fifo_states}=    Get FIFO Information
    Log    Initial FIFO States: ${initial_fifo_states}
    
    # Check and enable disabled FIFOs 0-4
    FOR    ${fifo_id}    IN RANGE    5    # FIFOs 0-4
        ${fifo_state}=    Set Variable    ${initial_fifo_states}[${fifo_id}]
        ${fifo_name}=    Set Variable    ${fifo_state}[name]
        Log    Checking FIFO ${fifo_id} (${fifo_name}) - Current State: ${fifo_state}[enabled]
        
        IF    not ${fifo_state}[enabled]
            Log    Enabling FIFO ${fifo_id} (${fifo_name})
            Enable FIFO    ${fifo_id}
        ELSE
            Log    FIFO ${fifo_id} (${fifo_name}) is already enabled
        END
    END
    
    # Execute reset command
    ${reset_cmd}=    Set Variable    snsrfifo reset
    @{reset_response}=    Create List    Fifos have been reset
    Log    Executing reset command: ${reset_cmd}
    Run Sensor Fifo CLI Test    ${reset_cmd}    Ok    ${reset_response}
    Log    Reset command executed successfully
    
    # Get final FIFO states with single lprop command
    ${post_reset_states}=    Get FIFO Information
    Log    FIFO States after reset: ${post_reset_states}
    
    # Verify all FIFOs 0-4 are disabled after reset
    FOR    ${fifo_id}    IN RANGE    5    # FIFOs 0-4
        ${fifo_state}=    Set Variable    ${post_reset_states}[${fifo_id}]
        ${fifo_name}=    Set Variable    ${fifo_state}[name]
        Should Not Be True    ${fifo_state}[enabled]    
        ...    FIFO ${fifo_id} (${fifo_name}) should be disabled after reset
        Log    Verified FIFO ${fifo_id} (${fifo_name}) is disabled after reset
    END