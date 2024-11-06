# Copyright (c) Microsoft Corporation. All rights reserved.


*** Settings ***
Documentation    Verifies the PCIe enumeration of SDM and CDED virtual functions from AP core 0.

Library     ${CURDIR}/../../pylibs
Library     pylibs.kng_pythia_libs.APBaremetalTests
...         name=pcie_vf_tests_logs
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/mscp_runtime_fw.json
...         WITH NAME    APBaremetalTestsLib

Suite Setup        Environment Setup
Suite Teardown     Test Teardown


*** Variables ***
@{PASS_RESPONSE}             Starting PCIe VFs Test
@{FAIL_RESPONSE}             FAIL
${PCIE_VF_CMD}               ap_bm pcie_vfs_test  
${READ UNTIL}                **** PCIe VFs Test DONE ****


*** Test Cases ***
Test Case - AP: Test PCIe Enumeration Test: []
    [Documentation]      Verifies the PCIe enumeration of SDM and CDED virtual functions from AP core 0.
    Send Test Command    ${PCIE_VF_CMD} 
    ${test_result}    Parse Test Output    ${READ UNTIL}    @{PASS_RESPONSE}    ${FAIL_RESPONSE}
    Should Be True      ${test_result}



*** Keywords ***
Environment Setup
   ${test_lib}    Get Library Instance      APBaremetalTestsLib
   ${connection}    Test Setup
    