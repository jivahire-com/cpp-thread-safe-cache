# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests the HeartBeat from MSCP via UART.
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class mscp_heart_beat_dual_die(EchoFallsBaseTest):

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

        connections = [{"entity": "scp_0", "channel": self.dut.mb.node_0.soc.primary_die.scp.channel_manager}, {"entity": "scp_1", "channel": self.dut.mb.node_0.soc.secondary_die.scp.channel_manager},
                        {"entity": "mcp_0", "channel": self.dut.mb.node_0.soc.primary_die.mcp.channel_manager}, {"entity": "mcp_1", "channel": self.dut.mb.node_0.soc.secondary_die.mcp.channel_manager}]
        
        # Ensure the host config file used alongside this test has these connections defined.
        for connection in connections:
            assert connection is not None
        
        self.dut.setup()
        
        for connection in connections:
            connection["channel"].get_current_channel().open()

            # If connection does not open then SVP didn't launch or FPGA system has a conflict booking. So teardown and return fail
            if not connection["channel"].get_current_channel().is_open():
                self.dut.teardown()
                time.sleep(30)
                return False
        
        for connection in connections:
            try:
                self.log.info(f"Reading from {connection['entity']}")

                # Clear buffer and read until HeartBeat
                connection["channel"].get_current_channel().read_until(key="HeartBeat", timeout_seconds=1800)
            except Exception as e:
                self.log.error(f"Error reading {connection['entity']} UART: {e}")
                self.test_notify(step="HeartBeat", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False

        self.test_notify(step="HeartBeat", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True
