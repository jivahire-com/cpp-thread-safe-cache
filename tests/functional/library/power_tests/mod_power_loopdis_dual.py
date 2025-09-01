# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test to ensure power loopdis_dual functionality
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest


# Class name must match file name for Robot Framework Library usage
class mod_power_loopdis_dual(EchoFallsBaseTest):

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
        name: str = "Power loopdis_dual test",
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

    def power_loopdis_dual_test(self):
        """
        Power Module loopdis_dual CLI test:
            1. Setup the Test.
            2. Execute Power module loopdis_dual CLI
            3. Check for pass criteria in  SCP UART log
            3. Teardown Test.
        """
        self.log.info("Running power loopdis dual CLI command Test. . .")
        self.dut.setup()
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)
            
        core_com_die0_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_die1_channel=self.dut.mb.node_0.soc.secondary_die.scp.channel_manager.get_current_channel()
        
        self._open_channels(core_com_die0_channel, core_com_die1_channel)
        
        try:
            self.log.info("Waiting for Heartbeat Msg")
            core_com_die0_channel.read_until(key="ScpHeartBeat", timeout_seconds=1800)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False
        
        try:
            self.log.info("Waiting for Die 1 Heartbeat Msg")
            core_com_die1_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.secondary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        try:
            core_com_die0_channel.write_line(write_string="pwr set loopdis 1 dual")
            core_com_die0_channel.read_until(key="Remote die loop disables Cmd Sent successfully.", timeout_seconds=30)
        except Exception as e:
            core_com_die0_channel.close()
            core_com_die0_channel.close()
            self.test_notify(step="pwr set loopdis 1 dual", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.test_notify(step="Power loopdis_dual", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True

    def _open_channels(self, *channels):
        for channel in channels:
            channel.open()
            assert channel.is_open()
