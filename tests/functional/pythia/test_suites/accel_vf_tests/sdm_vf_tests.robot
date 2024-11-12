# Copyright (c) Microsoft Corporation. All rights reserved.


*** Settings ***
Documentation    Test the SDM VF functionality completes from AP (Core 0)

Library     ${CURDIR}/../../pylibs
Library     pylibs.kng_pythia_libs.APBaremetalTests
...         name=sdm_vf_tests_logs
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    APBaremetalTestsLib

Suite Setup        Environment Setup
Suite Teardown     Test Teardown


*** Variables ***
@{PASS_RESPONSE}             wait_for_cmd_completion-: INFO: RX: SUCCESS
...                          compare_data_buffer: INFO: SUCCESS
...                          sdm_memcpy_test-: ****  SDM MEMCPY Test on Die 0 DONE ****
...    
@{FAIL_RESPONSE}             FAIL
${SDM_MEMCPY_DIE0_VF_CMD}    ap_bm sdm_memcpy_test   
${READ UNTIL}                SDM MEMCPY Test on Die 0 DONE


*** Test Cases ***
Test Case - AP: SDM single VF memcpy using the virtual function On Die 0
    [Documentation]     Test the SDM memcpy for virtual function.
    [Tags]   1423960
    Send Test Command    ${SDM_MEMCPY_DIE0_VF_CMD}
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE}       ${FAIL_RESPONSE}
    Should Be True      ${test_result}

Test Case - AP: SDM ALL VF memcpy Test Using WQ And VF Parameters
    [Documentation]     Test the SDM for all 31 virtual function and all 3 wq memcpy on SDM from AP core 0.
    [Tags]   1423960
    FOR    ${vf}    IN RANGE    0    32    
           FOR    ${wq}    IN RANGE    0    4 
               Send Test Command    ${SDM_MEMCPY_DIE0_VF_CMD} 0 ${wq} ${vf}
               ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE}       ${FAIL_RESPONSE}
               Should Be True      ${test_result}
            END   
    END
    

*** Keywords ***
Environment Setup
   ${test_lib}    Get Library Instance      APBaremetalTestsLib
   ${connection}    Test Setup
    