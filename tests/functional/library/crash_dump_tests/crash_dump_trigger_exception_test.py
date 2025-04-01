# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests Crash Dump Die0.
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
class crash_dump_trigger_exception_test(EchoFallsBaseTest):

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
        name: str = "crash_dump_test",
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
    
    def crash_dump_trigger_exception_test(self):
        """
        Crash Dump test:
            1. Setup the Test.
            2. Executes Crash Dump cli command.
            3. Teardown Test.
        """
        self.log.info("Running Crash Dump tests on Die0  . . .")
        self.dut.setup()

        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)
            
        scp_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        # Open SCP channel
        scp_channel.open()
        if not scp_channel.is_open():
            scp_channel.close()
            self.test_notify(step="Open_Channels", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        try:
            # Wait for ScpHeartBeat Completion message and enter commands
            self.log.info("Waiting for ScpHeartBeat  Msg")
            scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False


        command = "crashdump trig_except"
        self.log.info(f"Submitting {command}\n")
        scp_channel.write_line(write_string=command)
        try:
            scp_channel.read_until(key="BugCheck: 0x80380005", timeout_seconds=30)
        except Exception as e:
            self.log.error(f"Error Crash Dump. Die0: {e}")
            self.test_notify(step="Crash Dump.", msg="Test Fail", _is_error=True)
            scp_channel.close()
            self.dut.teardown()
            time.sleep(30)
            return False 

        # Close connection to SCP
        self.test_notify(step="Crash Dump Die0", msg="Test Done", _is_error=False)
        scp_channel.close()
        self.dut.teardown()
        time.sleep(30)
        return True
