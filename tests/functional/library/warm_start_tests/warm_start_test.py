# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test to verify the warm start CLI commands.
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class warm_start_test(EchoFallsBaseTest):

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
        name: str = "Warm start CLI test",
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
    
    def warm_start_test(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for AP to be up
            3. Send Wswr command, followed by wsrd command
            4. Read response and check if data written is read back
            5. Submit wsreset CLI cmd and wait for SCP to successfully come up after a warm reset
            6. Teardown Test. 
        """
        self.log.info("Running Warm start CLI command Test. . .")
        self.dut.setup()
            
        core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_channel.open()
        assert core_com_channel.is_open()
        
        try:
            self.log.info("Waiting for Heartbeat Msg")
            core_com_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info("Submitting warm start data write command . . .") 
        command="warm_start wswr 1 10 11 12 13"
        core_com_channel.write_line(write_string=command)

        try:
            core_com_channel.read_until(key="Data put 1", timeout_seconds=900)
            self.log.info("Data written to ID 1")
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            core_com_channel.close()
            self.test_notify(step="wswr command failed", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False
            
        self.log.info("Submitting warm start data read command . . .") 
        command="warm_start wsrd 1"
        core_com_channel.write_line(write_string=command)

        try:
            core_com_channel.read_until(key="Data for 1 (CLI)", timeout_seconds=900)
            self.log.info("Data read from ID 1")
            core_com_channel.read_until(key="0000: 0a 0b 0c 0d", timeout_seconds=900)
            self.log.info("Data read from ID 1")
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            core_com_channel.close()
            self.test_notify(step="wsrd command failed", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False
        
        self.log.info("Submitting MSCP subsys reset command . . .") 
        command="warm_start wsreset subsys"
        core_com_channel.write_line(write_string=command)

        try:
            core_com_channel.read_until(key="[APcore] SSI shutdown, shutdown type 2", timeout_seconds=900)
            self.log.info("Mailbox cmd sent to HSP requesting MSCP subsys reset.. ")
            self.log.info("Waiting until SCP is released out of reset.. ")
            core_com_channel.read_until(key="Hello World - SCP!", timeout_seconds=1800)
            self.log.info("SCP released out of reset")
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            core_com_channel.close()
            self.test_notify(step="wsreset subsys command failed", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False
               
        core_com_channel.close()
        self.test_notify(step="Warm start CLI tests done", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True