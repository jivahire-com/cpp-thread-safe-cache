# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for D2D Mailbox Echo Command ServerASYNC-ClientSYNC.
"""

import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class d2d_mailbox_cli_test_echo_serverasync_clientsync(EchoFallsBaseTest):

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
        name: str = "D2DMBX_Echo_ServerASYNC_ClientSYNC",
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
    
    def d2d_mailbox_cli_test_echo_serverasync_clientsync(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for SCP to be up. Send 'ECHO' command 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running D2D Mailbox CLI Echo command Test. . .")
        self.dut.setup()

        core_com_die0_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_die1_channel=self.dut.mb.node_0.soc.secondary_die.scp.channel_manager.get_current_channel()
        
        self._open_channels(core_com_die0_channel, core_com_die1_channel)
        
        try:
            self.log.info("Waiting for Heartbeat Msg")
            core_com_die0_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        try:
            self.log.info("Waiting for Heartbeat Msg")
            core_com_die1_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        
        self._submit_d2dmbx_command(core_com_die0_channel, core_com_die1_channel)

        try:
            self._server_command(core_com_die0_channel)
            self._client_command(core_com_die1_channel)
            self.log.info("Die 0 to Die 1 ECHO Complete")
        except Exception as e:
            core_com_die0_channel.close()
            core_com_die1_channel.close()
            self.test_notify(step="D2DMBX ECHO Command", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        try:
            self._server_command(core_com_die1_channel)
            self._client_command(core_com_die0_channel)
            self.log.info("Die 1 to Die 0 ECHO Complete")
        except Exception as e:
            core_com_die0_channel.close()
            core_com_die1_channel.close()
            self.test_notify(step="D2DMBX ECHO Command", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False
            
        core_com_die0_channel.close()
        core_com_die1_channel.close()
        self.test_notify(step="D2DMBX ECHO Command", msg="Test Done", _is_error=False)
        self.dut.teardown()
        return True
    
    def _open_channels(self, *channels):
        for channel in channels:
            channel.open()
            assert channel.is_open()

    def _submit_d2dmbx_command(self, channel1, channel2):
        self.log.info(f"Submitting command-icc_d2dmbx on Die0...")
        channel1.write_line(write_string="icc_d2dmbx")
        self.log.info(f"Submitting command-icc_d2dmbx on Die1...")
        channel2.write_line(write_string="icc_d2dmbx")

    def _server_command(self, channel1):
        channel1.write_line(write_string="echo_serv")
        try:
            self.log.info("Reading server message")
            channel1.read_until(key="Recv Initiated: Status[0x0]", timeout_seconds=300)
            self.log.info("Request SENT Successfully from One Die to Another...")
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            raise

    def _client_command(self, channel2):
        channel2.write_line(write_string="echo_client_sync")
        try:
            self.log.info("Reading client message")
            channel2.read_until(key="Echo Test Passed", timeout_seconds=300)
            self.log.info("Request SENT Successfully from One Die to Another...")
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            raise