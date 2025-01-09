*** Settings ***
Documentation     Test for sensor FIFO overflow functionality
...               Verifies overflow behavior for a single FIFO

Resource          ${CURDIR}/../../../../commonResource/sensor_fifo_common.resource
Library           Collections
Library           BuiltIn

Suite Setup       Setup Test Environment
Suite Teardown    Teardown Test Environment

*** Test Cases ***
Test Single FIFO Overflow
    [Documentation]    Test overflow functionality for a single FIFO (SCP_MSG)
    [Tags]    ssi    sensor_fifo    overflow    single_fifo    TEST_CASE_ID:2143255
    
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
    
    ${FIFO_NAME}=    Set Variable    SCP_MSG

    # Get test library instance
    ${test_lib}=    Get Library Instance    sensor_fifo_cli_test_lib
    
    # Setup prerequisites
    ${prereq_status}=    Call Method    ${test_lib}    setup_fifo_prerequisites
    IF    not ${prereq_status}    Fail    Failed to set up FIFO test prerequisites
    
    # Run overflow test and capture result
    TRY
        ${status}=    Call Method    ${test_lib}    test_fifo_overflow    ${FIFO_NAME}
        
        # Log results
        Log    \n=================== ${FIFO_NAME} Overflow Test Results ===================    console=True
        Log    Testing FIFO: ${FIFO_NAME}    console=True
        
        IF    ${status}
            Log    ✅ Overflow test passed    console=True
        ELSE
            Log    ❌ Overflow test failed    console=True
            Fail    Overflow test failed for ${FIFO_NAME} FIFO
        END
        
        Log    ====================================================    console=True
        
    EXCEPT    AS    ${error}
        Log    Error testing ${FIFO_NAME} overflow: ${error}    ERROR
        Fail    Overflow test failed with error: ${error}
    END