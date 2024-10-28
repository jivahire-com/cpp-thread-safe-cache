# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for sdm virtual function memcpy completion from AP core0.
"""

import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))
from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from robot.api.deco import keyword
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class APBaremetalTests(EchoFallsBaseTest):

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


    @keyword("Test Setup")
    def test_setup(self):
        self.log.info("Running SDM memcpy Test. . .")
        self.apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager
        assert self.apns_connection is not None

        self.dut.setup()
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            KngPythiaTestSetup.reset_fpga_sideload_testfw(self.dut, self.log)
        self.apns_connection.get_current_channel().open()

        if not self.apns_connection.get_current_channel().is_open():
            self.dut.teardown()
            return False
        
    @keyword("Send Test Command")    
    def send_command(self, command):    
        self.log.info(f"Submitting {command} command . . .")
        return self.apns_connection.get_current_channel().write_line(write_string=command)

    @keyword("Parse Test Output")    
    def parse_output(self, read_until_key, expected_results, failed_results, timeout=30):    
        command_response=self.apns_connection.get_current_channel().read_until(
            key=read_until_key, timeout_seconds=timeout)
        test_result=KngPythiaTestIF.parse_log(
            pythia_log=self.log, uart_log=command_response, uart_pass_logs=expected_results, uart_fail_logs=failed_results)        
        return test_result     
    
    @keyword("Test Teardown")  
    def test_teardown(self):
        self.dut.teardown()