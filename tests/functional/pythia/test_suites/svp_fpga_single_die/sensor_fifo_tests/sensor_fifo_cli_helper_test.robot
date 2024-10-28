*** Settings ***
Documentation     Verifies snsrfifo cli helper command output check.
Resource          ${CURDIR}/../../../../../commonResource/sensor_fifo_common.resource

*** Variables ***
${COMMAND}        snsrfifo ?
${READ_UNTIL_KEY_STRING}    Display menu or usage information
@{PASS_RESPONSE_LIST}  rdreg    Read a Register
...               wrreg    Write a Register
...               lprop    List Fifo Properties
...               globalhwen    Set Global HW Enable
...               reset    Reset all Fifos
...               fifoen    Enable/Disable a Fifo
...               rdentry    Read an entry from a fifo
...               wrentry    Write an entry to a fifo
...               updstride    Update stride index
...               ..    Goto Main Menu
...               ?    Display menu or usage information

*** Test Cases ***
Test Case - Test Sensor Fifo CLI Check with List
    [Documentation]    Verifies snsrfifo cli helper command output check.
    [Tags]    ssi   ssi_sensor_fifo_cli_helper
    Run Sensor Fifo CLI Test    ${COMMAND}    ${READ_UNTIL_KEY_STRING}    ${PASS_RESPONSE_LIST}