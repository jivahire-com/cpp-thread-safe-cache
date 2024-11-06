*** Settings ***
Documentation     Tests for sensor FIFO CLI commands
...               Verifies various sensor FIFO CLI commands functionality

Resource          ${CURDIR}/../../../../../commonResource/sensor_fifo_common.resource

Suite Setup       Setup Test Environment
Suite Teardown    Teardown Test Environment

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