# Copyright (c) Microsoft Corporation. All rights reserved.

# Guidelines
# In robot files, all commands and parameters need to be separated by a minimum of 1 tab space. 
# Pythia returns an error if this is not followed

*** Settings ***
Documentation    Robot file to restore ROM and Fuses using HSPprepHSP.cmm

# Import the Python library. Class name must match filename when using file paths.
# Importing also calls __init__().
Library     ${CURDIR}/../../pylibs
Library     pylibs.kng_pythia_libs.APBaremetalTests
...         name=fpga_setup
...         workspace_config=${WORKSPACE_CONFIG}
...         default_log_home=${LOG_DIR}
...         fw_payload_path=${PAYLOAD_DIR}
...         host_config=${HOST_CONFIG_DIR}/ap_baremetal_embed_fw_single_die.json
...         WITH NAME    APBaremetalTestsLib

Suite Setup       Dut Setup
Suite Teardown    Dut Teardown

*** Variables ***

*** Test Cases ***
Test Case - 0.Reset/Restore FPGA ROM and Fuses
    [Documentation]     Reset and Restore FPGA to a known good state    
    FPGA Reset ROM+Fuses
