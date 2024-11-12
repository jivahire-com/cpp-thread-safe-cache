# Copyright (c) Microsoft Corporation. All rights reserved.

*** Settings ***
Documentation    Test the CDED functionality with all VFs and WQs from AP (Core 0)

Library     ${CURDIR}/../../pylibs
Library     pylibs.kng_pythia_libs.APBaremetalTests
...         name=cded_vf_tests_logs
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    APBaremetalTestsLib

Suite Setup    Enviroment Setup
Suite Teardown    Test Teardown


*** Variables ***

@{PASS_RESPONSE_1}                               compare_data_buffer: INFO: SUCCESS
...                                              [Info] cded_vfs_test-: ****  CDED VFs Test DONE ****

@{PASS_RESPONSE_2}                               cded_test-: ****  CDED Test DONE ****  
@{FAIL_RESPONSE}                                 FAIL


${COMMAND_CDED_DIE0_VF_BUNDLE}                   ap_bm cded_vfs_test

# Variables for compression decompression test
${COMMAND_CDED_DIE0_VF_COMP}                     ap_bm cded_test -o x
${COMMAND_CDED_DIE0_VF_COMP_DECOMP}              ap_bm cded_test -o cx
@{CX_Algo}                                       raw
...                                              gzip
...                                              zlib
...                                              xp9
...                                              xp10
...                                              snappy

# Variables for encryption decryption test
${COMMAND_CDED_DIE0_VF_ENCR_DECR_xed}            ap_bm cded_test -o xed
${COMMAND_CDED_DIE0_VF_ENCR_DECR_ged}            ap_bm cded_test -o ged
${READ UNTIL}           CDED VFs Test DONE


*** Test Cases ***
Test Case - AP: CDED VFs Test
    [Documentation]     Run all CDED tests with all VFs and WQs (Core 0).
    [Tags]   1615866
    Send Test Command    ${COMMAND_CDED_DIE0_VF_BUNDLE}
    ${test_result}    Parse Test Output    ${READ UNTIL}     ${PASS_RESPONSE_1}        ${FAIL_RESPONSE}    ${120}
    Should Be True      ${test_result}


Test Case - AP: CDED Compression Test all VFs and WQs Test
    [Documentation]     Test the CDED compression with all VFs and WQs from AP (Core 0).
    [Tags]   1615866
    FOR    ${vf}    IN RANGE    0    32    
           FOR    ${wq}    IN RANGE    0    4 
               Send Test Command    ${COMMAND_CDED_DIE0_VF_COMP} -w ${wq} -f ${vf}
               ${test_result}    Parse Test Output    CDED Test DONE        ${PASS_RESPONSE_2}       ${FAIL_RESPONSE}    ${120}
               Should Be True      ${test_result}
            END   
    END


Test Case - AP: CDED Compression Decompression Test all algorithms, vfs and wqs
    [Documentation]     Test the CDED Conpression Decompression with all algorithms, vfs and wqs completes from AP (Core 0).
    [Tags]   1615866  
    FOR    ${Algo}    IN    @{CX_Algo}
        FOR    ${vf}    IN RANGE    0    32    
            FOR    ${wq}    IN RANGE    0    4 
                Send Test Command    ${COMMAND_CDED_DIE0_VF_COMP_DECOMP} -a ${Algo} -w ${wq} -f ${vf}
                ${test_result}    Parse Test Output    CDED Test DONE        ${PASS_RESPONSE_2}       ${FAIL_RESPONSE}    ${120}
                Should Be True      ${test_result}
            END   
        END
    END

Test Case - AP: CDED all VFs and WQs Encryption-Decryption AES-XTS Test
    [Documentation]     Test the CDED encryption decryption using AES-XTS from AP (Core 0).
    [Tags]   1615866
    FOR    ${vf}    IN RANGE    0    32    
           FOR    ${wq}    IN RANGE    0    4 
               Send Test Command    ${COMMAND_CDED_DIE0_VF_ENCR_DECR_xed}
               ${test_result}    Parse Test Output    CDED Test DONE        ${PASS_RESPONSE_2}       ${FAIL_RESPONSE}    ${120}
               Should Be True      ${test_result}
            END   
    END

Test Case - AP: CDED all VFs and WQs Encryption-Decryption AES-GCM Test
    [Documentation]     Test the CDED encryption decryption using AES-GCM from AP (Core 0).
    [Tags]   1615866
    FOR    ${vf}    IN RANGE    0    32    
           FOR    ${wq}    IN RANGE    0    4 
               Send Test Command    ${COMMAND_CDED_DIE0_VF_ENCR_DECR_ged} -w ${wq} -f ${vf}
               ${test_result}    Parse Test Output    CDED Test DONE        ${PASS_RESPONSE_2}       ${FAIL_RESPONSE}    ${120}
               Should Be True      ${test_result}
            END   
    END

*** Keywords ***
Enviroment Setup
   ${test_lib}    Get Library Instance      APBaremetalTestsLib
   ${connection}    Test Setup
    