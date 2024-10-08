# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for HSP Mailbox SEND RECV Command.
"""

import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class hsp_mailbox_cli_test_send_recv(EchoFallsBaseTest):

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
    
    def hsp_mailbox_cli_test_send_recv(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for SCP to be up. Send 'SEND' and 'RECV' command 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running HSP Mailbox CLI command Test. . .")
        self.dut.setup()

        core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()

        if (self.dut.get_dut_type() == DeviceType.BIGFPGA):
            KngPythiaTestSetup.reset_fpga_load_prodfw(self)
        
        elif (self.dut.get_dut_type() == DeviceType.SVP):
            core_com_channel.open()
            core_com_channel.is_open()

        else:
            self.log.error("Unsupported DUT type")
            self.dut.teardown()
            return False
        
        try:
            self.log.info("Waiting for Heartbeat Msg")
            core_com_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        self.log.info("Submitting HSP Mailbox command . . .") 
        command="icc_hspmbx"
        core_com_channel.write_line(write_string=command)

        for attempt in range(2):
            command="recv 193"
            self.log.info(f"Submitting RECV command {command} . . .") 
            core_com_channel.write_line(write_string=command)
            command="send 192 A B C"
            self.log.info(f"Submitting SEND command {command} . . .") 
            core_com_channel.write_line(write_string=command)

            try:
                core_com_channel.read_until(key="SCP->HSP Send Complete: Status[0x0]", timeout_seconds=900)
                self.log.info("Request SENT Successfully from SCP to HSP . . .")
                core_com_channel.read_until(key="SCP->HSP Recv Complete: Status[0x0]", timeout_seconds=900)
                self.log.info("Received Response Successfully from HSP and SCP . . .")
            except Exception as e:
                self.log.error(f"Error reading SCP UART: {e}")
                core_com_channel.close()
                self.test_notify(step="HSPMBX SEND RECV Command", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                return False
            
        core_com_channel.close()
        self.test_notify(step="HSPMBX SEND RECV Command", msg="Test Done", _is_error=False)
        self.dut.teardown()

        return True