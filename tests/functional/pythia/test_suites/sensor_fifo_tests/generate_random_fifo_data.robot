*** Settings ***
Resource          ${CURDIR}/../../../../commonResource/sensor_fifo_common.resource
Library           ${CURDIR}/../../pylibs/sensor_fifo_tests/sensor_fifo_lib.py
Library           ${CURDIR}/../../pylibs/sensor_fifo_tests/sensor_data_generator.py

*** Variables ***
${RANDOM_SEED}    ${52}    # More explicit seed value
${NUM_ENTRIES}    ${5}       # Number of entries to generate per FIFO

*** Keywords ***
Create Generator With Seed
    [Arguments]    ${seed}
    ${seed}=    Convert To Integer    ${seed}
    ${generator}=    Evaluate    sensor_data_generator.SensorDataGenerator(${seed})    modules=sensor_data_generator
    [Return]    ${generator}

Generate And Log FIFO Data
    [Arguments]    ${generator}    ${fifo_type}    ${num_entries}=${NUM_ENTRIES}
    ${num_entries}=    Convert To Integer    ${num_entries}
    ${data}=    Call Method    ${generator}    generate_sensor_fifo_data    ${fifo_type}    ${num_entries}
    Log    \n=== ${fifo_type} Data ===    console=True
    FOR    ${index}    ${entry}    IN ENUMERATE    @{data}    start=1
        Log    \nEntry ${index}:    console=True
        FOR    ${qword}    IN    @{entry}
            Log    ${qword}    console=True
        END
    END
    [Return]    ${data}

Compare FIFO Entries
    [Arguments]    ${data}
    ${length}=    Get Length    ${data}
    FOR    ${i}    IN RANGE    ${length}
        ${entry1}=    Set Variable    ${data}[${i}]
        FOR    ${j}    IN RANGE    ${i + 1}    ${length}
            ${entry2}=    Set Variable    ${data}[${j}]
            Should Not Be Equal    ${entry1}    ${entry2}    Duplicate entries found in FIFO data
        END
    END

*** Test Cases ***
Generate Random FIFO Data
    [Documentation]    Generates random data for each sensor FIFO type
    ...    The test uses a fixed seed (${RANDOM_SEED}) for reproducible results
    ...    and generates ${NUM_ENTRIES} entries per FIFO type.
    [Tags]    ssi    ssi_sensor_fifo    TEST_CASE_ID:xxxxxxxx
    
    # Test-specific variables
    @{FIFO_TYPES}=    Create List
    ...    PSTATE
    ...    SCP_MSG
    ...    TEMPERATURE
    ...    VOLTAGE
    ...    CURRENT
    ...    PVT_TEMP
    ...    PVT_VOLTAGE
    ...    DIMM_TEMP
    ...    VR_TEMP
    ...    VR_CURRENT
    
    ${generator}=    Create Generator With Seed    ${RANDOM_SEED}
    
    FOR    ${fifo_type}    IN    @{FIFO_TYPES}
        ${data}=    Generate And Log FIFO Data    ${generator}    ${fifo_type}
        Compare FIFO Entries    ${data}
        Log    \nVerified: All entries for ${fifo_type} are unique    console=True
    END