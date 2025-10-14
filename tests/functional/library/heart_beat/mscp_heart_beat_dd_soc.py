# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests the HeartBeat from MSCP via UART.
"""
import time
import sys, os
import subprocess
import json
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))
from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from RscmHelperLibrary import RscmHelperLibrary

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class mscp_heart_beat_dd_soc(EchoFallsBaseTest):

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
        name: str = "MSCP_HeartBeat_DualDie",
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
    
    def mscp_heartbeat(self):
        """
        MSCP heartbeat test:
            1. Setup the Test.
            2. Look for the SCP HeartBeat.
            2. Look for the MCP HeartBeat.
            3. Teardown Test.
        """
        self.log.info("Running MSCP heartBeat test . . .")
        
        self.dut.setup()
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)
        elif self.dut.get_dut_type() == DeviceType.RVP:
            self.log.warning("Device type is RVP. Performing SoC Reset ...")
            cred_path = os.environ.get('SECURE_FILE_PATH')
            creds = self.load_credentials_from_yaml(cred_path)
            rscm_helper = RscmHelperLibrary(rm_host=self.host_config.rack_scm.host, bmc_host=self.dut.mb.node_0.dcscm.bmc.ip, rm_user=creds['RM_USER'], rm_password=creds['RM_PASSWORD'], bmc_user=creds['BMC_USER'], bmc_password=creds['BMC_PASSWORD'], node=self.host_config.node_id)
            rscm_helper.rscm_soc_reset()

        core_scp_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_mcp_channel=self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
        for channel in [core_scp_channel, core_mcp_channel]:
            channel.open()
            assert channel.is_open()

        try:
            self.run_bmc_commands(["gpioset gpiochip3 5=0", "gpioset gpiochip6 6=0"])
            self.log.info(f"Reading from self.dut.mb.node_0.soc.primary_die.scp.channel_manager\n")
            core_scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=300)

            self.run_bmc_commands(["gpioset gpiochip3 5=1", "gpioset gpiochip6 6=0"])
            self.log.info(f"Reading from McpHeartBeat after BMC MUX\n")
            core_mcp_channel.read_until(key="McpHeartBeat", timeout_seconds=300)

        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="HeartBeat", msg="Test Fail", _is_error=True)
            self.run_bmc_commands(["gpioset gpiochip3 5=0", "gpioset gpiochip6 6=0"])
            self.dut.teardown()
            time.sleep(30)
            return False

        self.run_bmc_commands(["gpioset gpiochip3 5=0"])
        self.test_notify(step="HeartBeat", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)
        return True

    @staticmethod
    def load_credentials_from_yaml(path):
        import yaml
        with open(path, 'r') as f:
            data = yaml.safe_load(f)
        return data
    
    def run_bmc_commands(self, commands):
        # Ensure BMC CLI is open
        bmc_cli = self.dut.mb.node_0.dcscm.bmc.cli
        if not bmc_cli.is_open():
            bmc_cli.open()
        for cmd in commands:
            ret, stdout, stderr = bmc_cli.execute_command(command=cmd, command_args=[])
            if stderr:
                self.log.error(f"BMC command failed: {cmd} | Error: {stderr}")
                raise AssertionError
            self.log.info(f"BMC command successful: {cmd} | Return: {ret}")
