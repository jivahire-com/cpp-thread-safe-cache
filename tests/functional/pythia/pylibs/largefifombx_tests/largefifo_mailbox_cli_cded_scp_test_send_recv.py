# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for Large FIFO Mailbox CDED SCP SEND RECV Command.
"""

import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class largefifo_mailbox_cli_cded_scp_test_send_recv(EchoFallsBaseTest):

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
        name: str = "LargeFIFOMBX_CDED_SCP_Send_Recv",
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
    
    def largefifo_mailbox_cli_cded_scp_test_send_recv(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for CDED to be up. Send 'SEND' and 'RECV' command 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running Large FIFO Mailbox CLI command Test. . .")
        scp_connection=self.dut.mb.node_0.soc.primary_die.scp.channel_manager
        cded_connection = self.dut.mb.node_0.soc.primary_die.sdm_cded.channel_manager

        self.dut.setup()

        if (self.dut.get_dut_type() == DeviceType.SVP):
            # TODO:  No Support. Pranjal to update status and its ADO reference
            self.log.info("TODO SVP Send Recv Tests")
            self.dut.teardown()
            return True
         
        scp_channel = scp_connection.get_current_channel()
        cded_channel = cded_connection.get_current_channel()
        # Open SCP and CDED channels
        self.log.info("OPEN_CHANNEL")
        result = self._open_channels(scp_channel, cded_channel)
        if result is False:
            scp_channel.close()
            cded_channel.close()
            self.test_notify(step="Open_Channels", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False
        
        try:
            self.log.info("Waiting for Heartbeat Msg")
            scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=500)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        # Submit commands on CDED channel to receive request from SCP
        self._submit_command(scp_channel, "icc_largembx")
        self._submit_command(scp_channel, "cded_recv 2")
        self._submit_command(cded_channel, "scp_mbx")

        try:
            self._send_and_receive_commands(cded_channel, scp_channel)
        except Exception as e:
            scp_channel.close()
            cded_channel.close()
            self.test_notify(step="Large FIFO MBX SEND RECV Command", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False
        
        # Close channels after completion
        scp_channel.close()
        cded_channel.close()
        self.test_notify(step="Large FIFO MBX SEND RECV Command", msg="Test Done", _is_error=False)
        self.dut.teardown()
        return True
    
    def _open_channels(self, *channels):
        for channel in channels:
            channel.open()
            if not channel.is_open():
                return False
        return True

    def _submit_command(self, submit_channel, command):
        self.log.info(f"Submitting command '{command}'")
        # Write command on specified channel
        submit_channel.write_line(write_string=command)

    def _send_and_receive_commands(self, send_channel, receive_channel):
        commands = ["scp_send 2"]
        for cmd in commands:
            self.log.info(f"Submitting command '{cmd}'...")
            send_channel.write_line(write_string=cmd)
            try:
                # Validate Send Complete message on SCP
                send_channel.read_until(key="Send Complete", timeout_seconds=100)
                self.log.info("Request SENT Successfully from CDED to SCP...")
                # Validate Send Received message on CDED
                receive_channel.read_until(key="Recv Complete", timeout_seconds=100)
                self.log.info("Received Response Successfully from CDED to SCP...")
            except Exception as e:
                self.log.error(f"Error reading UART: {e}")
                raise Exception("Failed to send or receive command.") from e