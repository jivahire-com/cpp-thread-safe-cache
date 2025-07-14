#
# Copyright (c) Microsoft Corporation. All rights reserved.
#

*** Settings ***
Documentation     Tests for sensor FIFO CLI commands
...               Verifies various sensor FIFO CLI commands functionality

Resource          ${CURDIR}/../../../commonResource/sensor_fifo_common.resource
Library           Collections
Library           BuiltIn

Suite Setup       Setup Test Environment
Suite Teardown    Teardown Test Environment

*** Keywords ***
Run FIFO Test
    [Arguments]    ${test_lib}    ${fifo_name}    ${random_seed}    ${num_entries}
    ${status}=    Set Variable    ${FALSE}
    ${details}=    Set Variable    ${None}
    
    TRY
        # Run the test and capture the result and details
        ${status}    ${details}=    Run Keyword    Call Method    ${test_lib}    
        ...    test_fifo_write_read_entries_with_improved_error_handling    
        ...    ${fifo_name}    ${random_seed}    ${num_entries}
        
        # Log detailed results for this FIFO
        Run Keyword    Log    \n=================== ${fifo_name} Test Results ===================    console=True
        Run Keyword    Log    Testing FIFO: ${fifo_name}    console=True
        Run Keyword    Log    Number of Entries: ${num_entries}    console=True
        Run Keyword    Log    \nEntry-by-Entry Results:    console=True
        
        # Process entry results
        ${entries_passed}=    Set Variable    ${0}
        ${entries_failed}=    Set Variable    ${0}
        
        FOR    ${result}    IN    @{details}
            ${entry_num}=    Set Variable    ${result}[0]
            ${entry_status}=    Set Variable    ${result}[1]
            ${written_data}=    Set Variable    ${result}[2]
            ${read_data}=    Set Variable    ${result}[3]
            
            IF    ${entry_status}
                ${entries_passed}=    Evaluate    ${entries_passed} + 1
                Run Keyword    Log    ✅ Entry ${entry_num} - Passed    console=True
            ELSE
                ${entries_failed}=    Evaluate    ${entries_failed} + 1
                Run Keyword    Log    ❌ Entry ${entry_num} - Failed    console=True
                Run Keyword    Log    Written Data: ${written_data}    console=True
                Run Keyword    Log    Read Data: ${read_data}    console=True
            END
        END
        
        # Calculate and log success rate
        ${success_rate}=    Evaluate    (${entries_passed} / ${num_entries}) * 100
        Run Keyword    Log    \nFIFO Summary:    console=True
        Run Keyword    Log    Passed Entries: ${entries_passed}    console=True
        Run Keyword    Log    Failed Entries: ${entries_failed}    console=True
        Run Keyword    Log    Success Rate: ${success_rate}%    console=True
        Run Keyword    Log    ====================================================    console=True
        
    EXCEPT    AS    ${error}
        Run Keyword    Log    Error testing ${fifo_name}: ${error}    ERROR
        ${status}=    Set Variable    ${FALSE}
        ${details}=    Set Variable    ${None}
    END
    
    [Return]    ${status}    ${details}

*** Test Cases ***
Test All FIFOs Write Read Entry Operations
    [Documentation]    Test write and read entry functionality for all available FIFOs
    [Tags]    ssi    sensor_fifo    write_read    all_fifos    TEST_CASE_ID:2142913
    
    # Test-specific variables
    # if you want to test a single FIFO, you can use the following line: (can be used for debugging)
    # @{FIFO_NAMES}=    Create List    PSTATE
    @{FIFO_NAMES}=    Create List    PSTATE    SCP_MSG    TEMPERATURE    VOLTAGE    CURRENT             
    ...              PVT_TEMP    PVT_VOLTAGE    DIMM_TEMP    VR_TEMP    VR_CURRENT
    ${RANDOM_SEED}=    Set Variable    52
    ${NUM_ENTRIES}=    Set Variable    ${1}
    
    # Initialize results tracking
    ${results}=    Create Dictionary
    ${passed_count}=    Set Variable    ${0}
    ${failed_count}=    Set Variable    ${0}
    
    # Get test library instance
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib

    # Setup prerequisites once for all FIFOs
    ${prereq_status}=    Run Keyword    Call Method    ${test_lib}    setup_fifo_prerequisites
    
    Run Keyword If    not ${prereq_status}    Fail    Failed to set up FIFO test prerequisites

    # Get total number of FIFOs
    ${fifo_count}=    Get Length    ${FIFO_NAMES}
    
    # Test each FIFO
    FOR    ${fifo_name}    IN    @{FIFO_NAMES}
        ${status}    ${details}=    Run Keyword    Run FIFO Test    ${test_lib}    ${fifo_name}    
        ...    ${RANDOM_SEED}    ${NUM_ENTRIES}
        
        # Update results dictionary and counters
        Set To Dictionary    ${results}    ${fifo_name}=${status}
        ${passed_count}=    Evaluate    ${passed_count} + (1 if ${status} else 0)
        ${failed_count}=    Evaluate    ${failed_count} + (0 if ${status} else 1)
    END
    
    # Generate and log overall summary report
    Run Keyword    Log    \n=================== Overall Test Summary ===================    console=True
    Run Keyword    Log    Total FIFOs tested: ${fifo_count}    console=True
    Run Keyword    Log    Passed: ${passed_count}    console=True
    Run Keyword    Log    Failed: ${failed_count}    console=True
    Run Keyword    Log    \nDetailed Results:    console=True
    
    FOR    ${fifo_name}    IN    @{FIFO_NAMES}
        ${status}=    Get From Dictionary    ${results}    ${fifo_name}
        ${icon}=    Set Variable If    ${status}    ✅    ❌
        Run Keyword    Log    ${icon} ${fifo_name}: ${status}    console=True
    END
    Run Keyword    Log    \n====================================================    console=True
    
    # Fail the test case if any FIFO test failed
    Run Keyword    Should Be Equal As Numbers    ${failed_count}    0    
    ...    msg=\n${failed_count} FIFO tests failed. Check the detailed logs above for specific failures.    
    ...    values=False