*** Settings ***
Documentation     Tests for sensor FIFO CLI commands
...               Verifies sensor FIFO CLI command functionality for a single FIFO

Resource          ${CURDIR}/../../../commonResource/sensor_fifo_common.resource

Suite Setup       Setup Test Environment
Suite Teardown    Teardown Test Environment

*** Test Cases ***
Test Single FIFO Write Read Entry Operations
    [Documentation]    Test write and read entry functionality for Single FIFO
    [Tags]    sensor_fifo    write_read    single_fifo    TEST_CASE_ID:2142913
    
    # Setup prerequisites once
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    ${prereq_status}=    Call Method    ${test_lib}    setup_fifo_prerequisites
    
    IF    not ${prereq_status}
        Fail    Failed to set up FIFO test prerequisites
    END
    
    # Test-specific variables
    # Available FIFO names:
    # PSTATE         - ID: 0 - P-state telemetry
    # SCP_MSG        - ID: 1 - SCP message telemetry
    # TEMPERATURE    - ID: 2 - Tile temperature telemetry
    # VOLTAGE        - ID: 3 - Tile voltage telemetry
    # CURRENT        - ID: 4 - Core current telemetry
    # PVT_TEMP      - ID: 5 - PVT temperature telemetry
    # PVT_VOLTAGE   - ID: 6 - PVT voltage telemetry
    # DIMM_TEMP     - ID: 7 - DIMM temperature telemetry
    # VR_TEMP       - ID: 8 - VR temperature telemetry
    # VR_CURRENT    - ID: 9 - VR current telemetry
    
    ${FIFO_NAME}=    Set Variable    VR_CURRENT
    ${NUM_ENTRIES}=    Set Variable    ${1}
    ${RANDOM_SEED}=    Set Variable    42
       
    # Test the FIFO
    ${status}    ${details}=    Call Method    ${test_lib}    test_fifo_write_read_entries_with_improved_error_handling    
    ...    ${FIFO_NAME}    ${RANDOM_SEED}    ${NUM_ENTRIES}
    
    # Generate and log summary report
    Log    \n=================== FIFO Test Summary ===================    console=True
    Log    Testing FIFO: ${FIFO_NAME}    console=True
    Log    Number of Entries: ${NUM_ENTRIES}    console=True
    Log    \nEntry-by-Entry Results:    console=True
    
    # Initialize counters
    ${passed}=    Set Variable    ${0}
    ${failed}=    Set Variable    ${0}
    
    # Process results
    FOR    ${result}    IN    @{details}
        ${entry_num}=    Set Variable    ${result}[0]
        ${entry_status}=    Set Variable    ${result}[1]
        ${written_data}=    Set Variable    ${result}[2]
        ${read_data}=    Set Variable    ${result}[3]
        
        # Update counters
        IF    ${entry_status}
            ${passed}=    Evaluate    ${passed} + 1
            Log    ✅ Entry ${entry_num} - Passed    console=True
        ELSE
            ${failed}=    Evaluate    ${failed} + 1
            Log    ❌ Entry ${entry_num} - Failed    console=True
            Log    Written Data: ${written_data}    console=True
            Log    Read Data: ${read_data}    console=True
        END
    END
    
    # Calculate success rate
    ${success_rate}=    Evaluate    (${passed} / ${NUM_ENTRIES}) * 100
    
    # Log summary statistics
    Log    \nSummary Statistics:    console=True
    Log    Total Entries: ${NUM_ENTRIES}    console=True
    Log    Passed: ${passed}    console=True
    Log    Failed: ${failed}    console=True
    Log    Success Rate: ${success_rate}%    console=True
    Log    \n====================================================    console=True
    
    # Fail the test case if any entries failed
    Should Be True    ${status}    
    ...    msg=FIFO test failed for ${FIFO_NAME}. ${failed} out of ${NUM_ENTRIES} entries failed validation.