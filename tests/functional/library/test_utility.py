# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python-based Pythia 2.0 Test.
-This module provides test case utilities for verifying the multiple functionality.
"""

import sys, os
from pathlib import Path
import time
import socket
import json

from  log_action_processing import LogActionProcessing
from test_setup_helper import TestSetupHelper
from pythia.tdk.echofalls.constants.dut_types import DeviceType

from robot.api.deco import keyword
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
from pythia.tdk.common.util.variables_store import VariablesStore


class test_utility(EchoFallsBaseTest):
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
    TIMEOUT_SECONDS = 600

    # Create a list of cores for Die 0
    DIE0_CORES = ["scp0", "sdm0", "sdm_cded0", "mcp0"]
    DIE1_CORES = ["scp1", "sdm1", "sdm_cded1", "mcp1"]

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

    @keyword("FPGA Reset ROM")
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

    @keyword("Dut Setup")
    def dut_test_setup_simple(self, die_config: str = "Die0"):
        """Simply set up the DUT, don't wait for Heartbeat"""
        self.log.info(f"Running DUT Setup for configuration (not waiting for heartbeat): {die_config}...")

        # Initialize connections
        self.init_connections(die_config)

        # Setup the DUT
        try:
            self.dut.setup()
        except Exception as setup_error:
            self.log.error(f"Error during DUT setup initialization: {setup_error}")
            assert False, f"DUT setup failed: {setup_error}"
    
    def init_connections(self, die_config: str):
        self.apns0_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager
        assert self.apns0_connection is not None, "APNS0 connection not found."
        if die_config == "Die0":
            for core in self.DIE0_CORES:
                # Strip the die number from the core
                core_name = core.split("0")[0]
                setattr(self, f"{core}_connection", self.dut.mb.node_0.soc.primary_die.get_core(core_name).channel_manager)
                assert getattr(self, f"{core}_connection") is not None, f"{core} connection not found."

        elif die_config == "Die1":
            for core in self.DIE1_CORES:
                # Strip the die number from the core
                core_name = core.split("1")[0]
                setattr(self, f"{core}_connection", self.dut.mb.node_0.soc.secondary_die.get_core(core_name).channel_manager)
                assert getattr(self, f"{core}_connection") is not None, f"{core} connection not found."

        elif die_config == "DualDie":
            for core in self.DIE0_CORES:
                # Strip the die number from the core
                core_name = core.split("0")[0]
                setattr(self, f"{core}_connection", self.dut.mb.node_0.soc.primary_die.get_core(core_name).channel_manager)
                assert getattr(self, f"{core}_connection") is not None, f"{core} connection not found."
            for core in self.DIE1_CORES:
                core_name = core.split("1")[0]
                setattr(self, f"{core}_connection", self.dut.mb.node_0.soc.secondary_die.get_core(core_name).channel_manager)
                assert getattr(self, f"{core}_connection") is not None, f"{core} connection not found."
        else:
            raise ValueError(f"Invalid die configuration: {die_config}")

    # TODO: < 2410861 > Change function arguments
    @keyword("Dut Setup APBM")
    def dut_test_setup(self, die_config: str = "Die0"):
        """
        Set up the DUT and verify the required connections.
        """
        self.log.info(f"Running DUT Setup for configuration with APBM: {die_config}...")

        # Initialize connections
        self.init_connections(die_config)
        # Setup the DUT
        try:
            self.dut.setup()
        except Exception as setup_error:
            self.log.error(f"Error during DUT setup initialization: {setup_error}")
            assert False, f"DUT setup failed: {setup_error}"

        total_timeout = 600
        iteration_timeout = 100
        timeout_count_max = int(total_timeout/iteration_timeout)

        # Open SCP Die 0 UART to monitor Boot
        self.scp0_connection.get_current_channel().open()
        assert self.scp0_connection.get_current_channel().is_open(), "SCP Die0 channel is not open."

        # Wait for the DUT to power up
        for count_timeouts in range(timeout_count_max):
            # Check if AP is powered up by querying CLI
            self.send_command(command="?", connection_type="apns0")

            try:
                command_response = self.apns0_connection.get_current_channel().read_until(
                    key="? all",
                    timeout_seconds=iteration_timeout,
                )
                # Break if we get a response on CLI
                break
            except Exception as e:
                count_timeouts = count_timeouts + 1
                if count_timeouts >= timeout_count_max:
                    self.log.error(f"Error during DUT setup: {e}")
                    assert False, f"Error reaching AP Power Up: {e}"
                else:
                    continue

        self.log.info("DUT setup completed successfully.")
        self.scp0_connection.get_current_channel().close()
        self.apns0_connection.get_current_channel().close()
        time.sleep(5)

    @keyword("Dut Setup UEFI")
    def dut_test_setup_uefi_boot(self, die_config: str = "Die0"):
        """
        Set up the DUT and verify the required connections.
        """
        self.log.info(f"Running DUT Setup for configuration for UEFI Boot: {die_config}...")

        # Initialize connections
        self.init_connections(die_config)
        # Setup the DUT
        try:
            self.dut.setup()
        except Exception as setup_error:
            self.log.error(f"Error during DUT setup initialization: {setup_error}")
            assert False, f"DUT setup failed: {setup_error}"

        # Open SCP Die 0 UART to monitor Boot
        self.scp0_connection.get_current_channel().open()
        assert self.scp0_connection.get_current_channel().is_open(), "SCP Die0 channel is not open."

        self.apns0_connection.get_current_channel().open()
        assert self.apns0_connection.get_current_channel().is_open(), "APNS Die0 channel is not open."

        try:
            command_response = self.apns0_connection.get_current_channel().read_until(
                key="UEFI Interactive Shell",
                timeout_seconds=1000,
            )
        except Exception as e:
            self.log.error(f"Error during DUT setup: {e}")
            assert False, f"Error reaching UEFI Shell: {e}"

        self.log.info("DUT setup completed successfully, loaded to UEFI Shell.")
        self.scp0_connection.get_current_channel().close()
        self.apns0_connection.get_current_channel().close()
        time.sleep(5)

    @keyword("Send Test Command")
    def send_command(self, command: str, connection_type: str):
        self.log.info(f"Submitting '{command}' command using '{connection_type}' connection...")
        # Select the appropriate connection
        connection = getattr(self, f"{connection_type}_connection", None)
        if not connection:
            raise ValueError(f"Invalid connection type: {connection_type}")

        connection.get_current_channel().open()
        assert connection.get_current_channel().is_open(), f"{connection_type} channel is not open."

        return connection.get_current_channel().write_line(write_string=command)
    
    @keyword("Parse Test Output")
    def parse_output(self, connection_type: str, read_until_key: str, expected_results: list, failed_results: list, check_numbers: bool = False, timeout: int = 500):
        """
        Parse the DUT output for validation.

        :param connection_type: The connection type to use ('scp0', 'sdm0', 'mcp0', 'apns0').
        :param read_until_key: Key to look for in the output.
        :param expected_results: List of strings indicating success.
        :param failed_results: List of strings indicating failure.
        :param check_numbers: Whether to consider numbers in the logs. Defaults to False.
        :param timeout: Timeout in seconds for waiting.
        :return: Parsed test result.
        """
        self.log.info(f"Parsing output using '{connection_type}' connection with key '{read_until_key}', timeout {timeout} seconds.")
        # Select the appropriate connection
        connection = getattr(self, f"{connection_type}_connection", None)
        if not connection:
            raise ValueError(f"Invalid connection type: {connection_type}")

        connection.get_current_channel().open()
        assert connection.get_current_channel().is_open(), f"{connection_type} channel is not open."

        try:
            # Read the output until the key or timeout occurs
            command_response = connection.get_current_channel().read_until(
                key=read_until_key,
                timeout_seconds=timeout,
            )
            self.log.info(f"Command response received: {command_response}")
            test_result = LogActionProcessing.parse_log(
                pythia_log=self.log,
                uart_log=command_response,
                uart_pass_logs=expected_results,
                uart_fail_logs=failed_results,
                ignore_numbers=check_numbers,
            )
            return test_result
        except Exception as e:
            self.log.error(f"Error during output parsing: {e}")
            return False

    @keyword("Dut Force Teardown")
    def test_teardown_force(self):
        """Force Teardown the DUT after the test"""
        self.log.info("Forcibly Tearing down the DUT...")
        VariablesStore()["FORCE_SVP_TEARDOWN"] = True
        self.dut.teardown()
        time.sleep(30)
        
    @keyword("Dut Teardown")
    def test_teardown(self):
        """Teardown the DUT after the test."""
        self.log.info("Tearing down the DUT...")
        self.dut.teardown()
        time.sleep(30)

    @keyword("OOB Reset")
    def fpga_oob_reset(self):
        #Create new file in the shared directory for the reset monitor listener
        self.log.info("Creating reset monitor listener file to trigger OOB reset...")
        fpga_system_name = socket.gethostname().lower()
        self.log.info(f"System Name: {fpga_system_name}")
        fpga_system_name = fpga_system_name.split('rdu-120015-')[-1]
        reset_monitor_file_path = Path("R:/haps_runtime/kingsgate_big_fpga/reset_tools/force_reset_" + fpga_system_name)
        reset_monitor_file = reset_monitor_file_path / "reset"
        new_monitor_file = reset_monitor_file_path / ".newer"

        if not new_monitor_file.exists():
            new_monitor_file.touch()
            self.log.info(f"Newer file created at: {new_monitor_file}")

        if reset_monitor_file.exists():
            reset_monitor_file.unlink()
            self.log.info(f"Reset file deleted at: {reset_monitor_file}")
        time.sleep(5)

        reset_monitor_file.touch()
        self.log.info(f"Reset file created at: {reset_monitor_file}")
        time.sleep(5)
        reset_monitor_file.unlink()
        self.log.info(f"Reset file deleted at: {reset_monitor_file}")
