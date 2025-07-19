# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test to execute power module functional tests
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
import re

# Class name must match file name for Robot Framework Library usage
class mod_power_ft(EchoFallsBaseTest):

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
        name: str = "Power module functional tests",
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

    def power_module_functional_test(self):
        """
        Power Module control loop CLI test:
            1. Setup the Test.
            2. Execute the functional tests with power module CLIs
            3. Check for pass criteria in  SCP UART log
            4. Teardown Test.
        """
        self.log.info("Running Power module functional tests  . . .")
        self.dut.setup()
            
        core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_channel.open()
        assert core_com_channel.is_open()

        try:
            self.log.info("Waiting for boot complete message on SCP UART")
            core_com_channel.read_until(key="SOS boot completed", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="Boot complete", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info("Submitting power module pwr set nominal command . . .")
        command="pwr set nominal 1"
        core_com_channel.write_line(write_string=command)
        try:
            command_response_cli = core_com_channel.read_until(key="nominal pstate", timeout_seconds=300)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            self.test_notify(step="Power module pwr set nominal cmd status: Fail", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        pattern = re.compile(r"nominal pstate: (\w+) \(previous (\w+)\)")
        match = pattern.search(command_response_cli)

        if match:
            current_pstate = match.group(1)
            previous_pstate = match.group(2)
            self.log.info(f"Current P-State: {current_pstate}, Previous P-State: {previous_pstate}")
        else:
            self.log.error(f"pstate set to: {current_pstate}")
            self.test_notify(step="Nominal pstate not set successfully", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info("Submitting power module pwr cfg <args> commands . . .")
        commands = ["pwr cfg tel"]
        for command in commands:
            self.log.info(f"Submitting {command}\n")
            core_com_channel.write_line(write_string=command)
            try:
                core_com_channel.read_until(key="Telemetry config", timeout_seconds=300)
            except Exception as e:
                self.log.error(f"Error reading SCP UART: {e}")
                self.test_notify(step="Power module pwr cfg <args> cmd status: Fail", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False


        self.log.info("Submitting power module pwr set <args> commands . . .")
        command="pwr set cap 65535"
        core_com_channel.write_line(write_string=command)
        try:
            command_response_cli = core_com_channel.read_until(key="success", timeout_seconds=300)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            self.test_notify(step="Power module pwr set cap cmd status: Fail", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        pattern = re.compile(r"pwr_set cap: current - (\d{5}W)")
        match = pattern.search(command_response_cli)

        if match:
            val = match.group(1)
            self.log.info("Pwr cap set successfully")
        else:
            self.log.error(f"Power cap set to: {val}W")
            self.test_notify(step="Power cap not set successfully", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        command="pwr set minupdate 0"
        core_com_channel.write_line(write_string=command)
        try:
            command_response_cli = core_com_channel.read_until(key="min_plimit_update", timeout_seconds=300)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            self.test_notify(step="Power module pwr set desired cmd status: Fail", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        pattern = re.compile(r"min_plimit_update: (\d+)")
        match = pattern.search(command_response_cli)

        if match:
            value = match.group(1)
            self.log.info("set the minimum plimit update per loop iteration to 0")
        else:
            self.log.error(f"Min plimit update not set")
            self.test_notify(step="Min plimit update not set", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        command="pwr set plimit 1 31"
        core_com_channel.write_line(write_string=command)
        try:
            command_response_cli = core_com_channel.read_until(key="pwr set plimit: core", timeout_seconds=300)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            self.test_notify(step="Power module pwr set plimit cmd status: Fail", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        pattern = re.compile(r"pwr set plimit: core - (\d+) \(0x([0-9A-Fa-f]+)\) plimit - 0x([0-9A-Fa-f]+)")
        match = pattern.search(command_response_cli)

        if match:
            core = match.group(1)
            core_addr = match.group(2)
            desired_val = match.group(3)
            if not (core == "1" and core_addr == "60400000" and desired_val == "1f"):
                self.log.error(f"Core: {core}, Addr: 0x{core_addr}, Desired value: 0x{desired_val}")
                self.test_notify(step="plimit not set", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            print(f"Core: {core}, Addr: 0x{core_addr}, Desired value: 0x{desired_val}")
        else:
            self.log.error(f"plimit update not set")
            self.test_notify(step="plimit update not set", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        command="pwr set desired 0 0 1"
        core_com_channel.write_line(write_string=command)
        try:
            command_response_cli = core_com_channel.read_until(key="throttle_pri -", timeout_seconds=300)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            self.test_notify(step="Power module pwr set desired cmd status: Fail", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        pattern = re.compile(r"pwr set desired: core - (\d+) \(0x([0-9A-Fa-f]+)\) desired - 0x([0-9A-Fa-f]+) throttle_pri - 0x([0-9A-Fa-f]+)")
        match = pattern.search(command_response_cli)

        if match:
            value = match.group(1)
            self.log.info(f"Core: {value}")
        else:
            self.log.error(f"Desired pstate not set for core 0")
            self.test_notify(step="Desired pstate not set for core 0", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        # TODO: fix test case,   https://azurecsi.visualstudio.com/Dev/_workitems/edit/2199252
        # command="pwr set plimit 0 1"
        # core_com_channel.write_line(write_string=command)
        # try:
        #     command_response_cli = core_com_channel.read_until(key="success message, core 0", timeout_seconds=300)
        # except Exception as e:
        #     self.log.error(f"Error reading SCP UART: {e}")
        #     self.test_notify(step="Power module pwr set plimit cmd status: Fail", msg="Test Fail", _is_error=True)
        #     self.dut.teardown()
        #     return False

        # pattern = re.compile(r"pwr set plimit: core - (\d+)")
        # match = pattern.search(command_response_cli)

        # if match:
        #     value = match.group(1)
        #     self.log.info(f"Core: {value}")
        # else:
        #     self.log.error(f"Desired plimit not set for core 0")
        #     self.test_notify(step="Desired plimit not set for core 0", msg="Test Fail", _is_error=True)
        #     self.dut.teardown()
        #     return False

        core_com_channel.close()
        self.test_notify(step="Power module functional tests complete", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True
