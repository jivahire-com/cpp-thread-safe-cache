# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for HSP Mailbox ECHO Die1 Command.
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class hsp_mailbox_cli_test_die1(EchoFallsBaseTest):

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
        name: str = "HSPMBX_Echo_test_die1",
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
    
    def hsp_mailbox_cli_test_die1(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for AP to be up. Send 'ECHO' command 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running HSP Mailbox CLI Die1 command Test. . .")
        self.dut.setup()
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)
            
        core_com_channel=self.dut.mb.node_0.soc.secondary_die.scp.channel_manager.get_current_channel()
        core_com_channel.open()
        assert core_com_channel.is_open()
        
        # Restriction: this needs to be run from a cold boot 
        try:
            self.log.info("Waiting for Boot complete / Heartbeat Msg")
            core_com_channel.read_until(key="SOS boot completed", timeout_seconds=1800)
            core_com_channel.read_until(key="ScpHeartBeat", timeout_seconds=200)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.secondary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info("Submitting MSP Mailbox ECHO command . . .") 
        command="icc_hspmbx"
        core_com_channel.write_line(write_string=command)

        for attempt in range(3):
            command="echo"
            self.log.info(f"Submitting ECHO command {command} . . .") 
            core_com_channel.write_line(write_string=command)

            try:
                core_com_channel.read_until(key="SCP->HSP Send Initiated: Status[0x0]", timeout_seconds=900)
                self.log.info("ECHO command Sent Successfully from SCP to HSP . . .")
                core_com_channel.read_until(key="SCP->HSP Recv Complete: Status[0x0]", timeout_seconds=900)
                self.log.info("ECHO command Received Successfully from HSP and SCP . . .")
            except Exception as e:
                self.log.error(f"Error reading SCP UART: {e}")
                core_com_channel.close()
                self.test_notify(step="HSPMBX ECHO Command", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            
        core_com_channel.close()
        self.test_notify(step="HSPMBX ECHO Command", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True