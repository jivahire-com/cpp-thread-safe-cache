# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for cded compress decompress completion from AP core0.
"""

import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))
from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class cded_comp_decomp_test(EchoFallsBaseTest):

    """
    :param name:                Name of the test case
    :param number:              ADO Number of the test case
    :param workspace_config:    Path to the workspace config file
    :param default_log_home:    Path for log storage
    :param fw_payload_path:     Path to the recipe payload
    :param dut_platform:        Platform used during the test case execution
    :param host_config:         Path to the host config file/directory
    :param host_name:           Name of the host to find the host config file, if host_config is a directory. Defaults to None
    """
    def __init__(
        self,
        name: str = None,
        number: str = "NaN",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = "KingsgateSVP",
        host_config: Path | str = None,
        host_name: str | None = None,
    ):
        
        # Call parent class init
        super().__init__(
            name,
            number,
            workspace_config,
            default_log_home,
            fw_payload_path,
            dut_platform,
            host_config,
            host_name,
        )
    
    def cded_comp_decomp_test(self, pass_logs, fail_logs):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for AP to be up. Send 'ap_bm cded_test' command 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running CDED compress decompress Test. . .")
        
        apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager
        assert apns_connection is not None

        self.dut.setup()

        if (self.dut.get_dut_type() == DeviceType.BIGFPGA):
            KngPythiaTestSetup.reset_fpga_sideload_testfw(self.dut, self.log)
        
        apns_connection.get_current_channel().open()
        # If unable to reach apns_connection, then FPGA system has a conflcit booking or SVP failed to launch. So return fail
        if not apns_connection.get_current_channel().is_open():
            self.dut.teardown()
            return False

        command="ap_bm cded_test"
        self.log.info(f"Submitting {command} command . . .")
        apns_connection.get_current_channel().write_line(write_string=command)

        try:
            command_response=apns_connection.get_current_channel().read_until(key="CDED Test DONE", timeout_seconds=300)
            test_result=KngPythiaTestIF.parse_log(pythia_log=self.log, uart_log=command_response, uart_pass_logs=pass_logs, uart_fail_logs=fail_logs)
        except Exception as e:
            self.log.error(f"Error reading UART: {e}")
            test_result = False      
            
        self.dut.teardown()
        return test_result
        