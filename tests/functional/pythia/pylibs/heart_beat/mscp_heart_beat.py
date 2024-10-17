# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests the HeartBeat from MSCP via UART.
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
class mscp_heart_beat(EchoFallsBaseTest):

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
        name: str = "MSCP_HeartBeat_Die0",
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

        scp_connection = self.dut.mb.node_0.soc.primary_die.scp.channel_manager
        mcp_connection = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager

        # Ensure the host config file used alongside this test has these connections defined.
        assert scp_connection is not None
        assert mcp_connection is not None

        self.dut.setup()

        scp_connection.get_current_channel().open()
        # If connection does not open then SVP didn't launch or FPGA system has a conflict booking. So teardown and return fail
        if not scp_connection.get_current_channel().is_open():
            self.dut.teardown()
            return False

        mcp_connection.get_current_channel().open()
        # If connection does not open then SVP didn't launch or FPGA system has a conflict booking. So teardown and return fail
        if not mcp_connection.get_current_channel().is_open():
            self.dut.teardown()
            return False


        self.log.info("Reading SCP UART for HeartBeat . . .")

        try:
            scp_connection.get_current_channel().read_until(key="HeartBeat", timeout_seconds=500)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            scp_connection.get_current_channel().close()
            self.test_notify(step="HeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        self.log.info("Reading MCP UART for HeartBeat . . .")

        try:
            mcp_connection.get_current_channel().read_until(key="HeartBeat", timeout_seconds=500)
        except Exception as e:
            self.log.error(f"Error reading MCP UART: {e}")
            mcp_connection.get_current_channel().close()
            self.test_notify(step="HeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        scp_connection.get_current_channel().close()
        mcp_connection.get_current_channel().close()
        self.test_notify(step="HeartBeat", msg="Test Done", _is_error=False)
        self.dut.teardown()

        return True
