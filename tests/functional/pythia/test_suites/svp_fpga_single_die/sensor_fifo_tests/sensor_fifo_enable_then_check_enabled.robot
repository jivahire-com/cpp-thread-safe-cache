		
*** Settings ***
Documentation     Verifies snsrfifo cli enable fifo id 0 and check enabled.
Resource          ${CURDIR}/../../../../../commonResource/sensor_fifo_common.resource

*** Variables ***
${OPTIONAL_FIRST_COMMAND}   snsrfifo fifoen 0 1
${COMMAND}  snsrfifo lprop
${READ_UNTIL_KEY_STRING}    Fifo ID = 1
${PASS_RESPONSE_STRING}   enabled = true


*** Test Cases ***
Test Case - Test Sensor Fifo enable fifo ID 0 and then check enabled
    [Documentation]    Verifies snsrfifo cli enable fifo id 0 and check enabled.
    [Tags]    ssi   ssi_sensor_fifo_enable_then_check_enabled
    Run Sensor Fifo CLI Test with optional first command    ${COMMAND}    ${READ_UNTIL_KEY_STRING}    ${PASS_RESPONSE_STRING}    ${OPTIONAL_FIRST_COMMAND}