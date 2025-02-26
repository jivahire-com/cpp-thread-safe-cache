# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python-based Pythia 2.0 Test.
-This module provides test case utilities for verifying the multiple functionality.
"""

import sys, os
from pathlib import Path
import time

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

    READ_UNTIL_KEY = "Primary AP core power on"
    TIMEOUT_SECONDS = 900

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

    @keyword("FPGA Force Reset")
    def reset_fpga(self):

        script_path = "R:/Kingsgate/Kingsgate_TRACE32/Prep_R22/"
        script_name = "HSPresetSystem.cmm"
        script = Path(r"{}".format(os.path.join(script_path, script_name)))

        self.log.info("Forcefully resetting FPGA . . .")
        self.log.info(f"Calling HSP CMM Script to reset SoC: {script}")
        hsp = self.dut.mb.node_0.soc.primary_die.get_core("hsp")
        hsp.debugger.execute_script(script)
        
        # Wait 5 seconds for the script to run. This is the worst case delay for the script to complete
        time.sleep(5)

        self.log.info("Resetting FPGA Done!!!")

    @keyword("FPGA Reset ROM+Fuses")
    def reset_fpga_fuses(self):

        script_path = "R:/Kingsgate/Kingsgate_TRACE32/Prep_R22/"
        script_name = "HSPprepHSP.cmm"
        script = Path(r"{}".format(os.path.join(script_path, script_name)))

        self.log.info("Restoring FPGA ROM and Fuses . . .")
        self.log.info(f"Calling HSP CMM Script to restore ROM and Fuses: {script}")
        hsp = self.dut.mb.node_0.soc.primary_die.get_core("hsp")
        hsp.debugger.execute_script(script)
        
        # Wait 20 seconds for the script to run. This is the worst case delay for the script to complete
        time.sleep(20)

        self.log.info("Restoring FPGA ROM and Fuses Done!!!")

    @keyword("Dut Setup Simple")
    def dut_test_setup_simple(self):
        """Set up the DUT, don't wait for Heartbeat"""
        self.log.info("Running DUT Setup (not waiting for heartbeat)...")

        try:
            self.dut.setup()
        except Exception as setup_error:
            self.log.error(f"Error during DUT setup initialization: {setup_error}")
            assert False, f"DUT setup failed: {setup_error}"
    
    @keyword("Dut Setup")
    def dut_test_setup(self):
        """Simply set up the DUT, don't wait for Heartbeat"""
        self.log.info("Running DUT Setup (not waiting for heartbeat)...")
        try:
            # Initialize connections
            self.scp_connection = self.dut.mb.node_0.soc.primary_die.scp.channel_manager
            assert self.scp_connection is not None, "SCP connection not found."

            self.apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager
            assert self.apns_connection is not None, "APNS connection not found."

            self.dut.setup()

            # Open connections
            self.scp_connection.get_current_channel().open()
            self.apns_connection.get_current_channel().open()

            if not self.scp_connection.get_current_channel().is_open():
                self.log.error("SCP channel failed to open.")
                return False

            if not self.apns_connection.get_current_channel().is_open():
                self.log.error("APNS channel failed to open.")
                return False
            
            # Wait for the DUT to power up
            self.log.info(f"Waiting for key '{self.READ_UNTIL_KEY}' within {self.TIMEOUT_SECONDS} seconds.")
            command_response = self.scp_connection.get_current_channel().read_until(
                key=self.READ_UNTIL_KEY,
                timeout_seconds=self.TIMEOUT_SECONDS,
            )
            time.sleep(5)

        except Exception as e:
            self.log.error(f"Error during DUT setup: {e}")
            assert False, f"Error reaching AP Power Up: {e}"

    @keyword("Send Test Command")
    def send_command(self, command: str):
        """Send a command to the DUT."""
        self.log.info(f"Submitting '{command}' command...")
        return self.apns_connection.get_current_channel().write_line(write_string=command)
    
    @keyword("Parse Test Output")
    def parse_output(self, read_until_key: str, expected_results: list, failed_results: list, timeout: int = 500):
        """
        Parse the DUT output for validation.

        :param read_until_key: Key to look for in the output.
        :param expected_results: List of strings indicating success.
        :param failed_results: List of strings indicating failure.
        :param timeout: Timeout in seconds for waiting.
        :return: Parsed test result.
        """
        self.log.info(f"Parsing output with key '{read_until_key}', timeout {timeout} seconds.")
        command_response = self.apns_connection.get_current_channel().read_until(
            key=read_until_key,
            timeout_seconds=timeout,
        )

        test_result = KngPythiaTestIF.parse_log(
            pythia_log=self.log,
            uart_log=command_response,
            uart_pass_logs=expected_results,
            uart_fail_logs=failed_results,
        )

        self.apns_connection.get_current_channel().clear_buffer()
        return test_result

    @keyword("Dut Teardown")
    def test_teardown(self):
        """Teardown the DUT after the test."""
        self.log.info("Tearing down the DUT...")
        self.dut.teardown()
        time.sleep(30)