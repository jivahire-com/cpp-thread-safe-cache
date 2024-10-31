# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test to execute telemetry loop health status
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
class mod_power_telemetry_loop_health(EchoFallsBaseTest):

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
        name: str = "Power telemetry loop health status test",
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
    
    def power_telemetry_loop_health_test(self):
        """
        Power Module control loop CLI test:
            1. Setup the Test.
            2. Execute the power module CLIs to test power telemetry health
            3. Check for pass criteria in  SCP UART log
            4. Teardown Test.
        """
        self.log.info("Running Power telemetry loop health test  . . .")
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
            return False
        
        self.log.info("Submitting power module pwr status <args> commands . . .") 
        commands = ["pwr status vrtl", "pwr status pvttl"]
        for command in commands:
            self.log.info(f"Submitting {command}\n")
            core_com_channel.write_line(write_string=command)
            try:
                core_com_channel.read_until(key="Health: no errors", timeout_seconds=30)
            except Exception as e:
                self.log.error(f"Error reading SCP UART: {e}")
                self.test_notify(step="Power module pwr status <args> cmd status: Fail", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                return False
        
        core_com_channel.close()
        self.test_notify(step="Power module functional tests complete", msg="Test Done", _is_error=False)
        self.dut.teardown()

        return True