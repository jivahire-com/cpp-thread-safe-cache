# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests Power control loop Die0.
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
class power_control_loop_health_test(EchoFallsBaseTest):

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
        name: str = "power_loop_test",
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
    
    def power_control_loop_health_test(self):
        """
        power control loop test:
            1. Setup the Test.
            2. Executes cli command and check test result no error.
            3. Teardown Test.
        """
        self.log.info("Running power control loop tests on Die0  . . .")
        self.dut.setup()

        if (self.dut.get_dut_type() == DeviceType.BIGFPGA):
            self.log.info("TODO BIGFPGA/RVP power control loop tests")
            self.dut.teardown()
            return True
        
        elif (self.dut.get_dut_type() == DeviceType.SVP):
            print("SVP device")
            core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
            core_com_channel.open()
            assert core_com_channel.is_open()
            

        else:
            self.log.error("Unsupported DUT type")
            self.dut.teardown()
            return False

        try:
            self.log.info(f"Reading from self.dut.mb.node_0.soc.primary_die.scp.channel_manager\n")
            core_com_channel.read_until(key="HeartBeat", timeout_seconds=900)
            time.sleep(20)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="HeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        connection = self.dut.mb.node_0.soc.primary_die.scp.channel_manager

        commands = ["pwr status cl","pwr status vrtl","pwr status pvttl"]
        for command in commands:
            self.log.info(f"Submitting {command}\n")
            core_com_channel.write_line(write_string=command)
            try:
                connection.get_current_channel().read_until(key="Health: no errors", timeout_seconds=30)
            except Exception as e:
                self.log.error(f"Error power loop. Die0: {e}")
                self.test_notify(step="Power loop health.", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                return False 

        self.test_notify(step="Power loop health Die0", msg="Test Done", _is_error=False)
        self.dut.teardown()
        return True
