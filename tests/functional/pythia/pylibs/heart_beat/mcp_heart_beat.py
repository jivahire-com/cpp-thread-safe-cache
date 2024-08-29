"""
- Python based Pythia 2.0 Test.
- Tests the HeartBeat from MCP via UART.
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
class mcp_heart_beat(EchoFallsBaseTest):

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

    def test_mcp_bl_embed_fw(self):
        """
        MCP Bootloader heartbeat test:
            1. Setup the Test.
            2. Look for the MCP Bootloader HeartBeat.
            3. Teardown Test.

            Note: This test is only applicable for SVP right now. On FPGA, it always returns PASS.
        """
        if (self.dut.get_dut_type() == DeviceType.SVP):
            self.dut.setup()
            # Ensure the host config file used alongside this test has these connections defined.
            assert self.dut.mb.node_0.soc.primary_die.mcp.channel_manager is not None
            self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel().open()
            assert self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel().is_open()

            mcp_lines = ' '.join(KngPythiaTestIF.read_and_log_lines(self,connection=self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel(), num_lines=30))
            test_result = True if ('HeartBeat' in mcp_lines) else False

            self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel().close()
            self.dut.teardown()

            # Notify of the test results. If test_pass is False it is an error (which also signals to the base test class that the test failed).
            self.test_notify(step="HeartBeat", msg="Test Done", _is_error=not(test_result))
            return test_result
        
        else:
            self.log.info("Test not implemented/applicable for this DUT. Bypassing, returning PASS")
            time.sleep(2)
            return True


    def mcp_heartbeat(self):
        """
        MCP heartbeat test:
            1. Setup the Test.
            2. Look for the MCP HeartBeat.
            3. Teardown Test.

            Note: This test is only applicable for SVP right now. On FPGA, it always returns PASS.
        """
        if (self.dut.get_dut_type() == DeviceType.SVP):
            self.dut.setup()

            # Ensure the host config file used alongside this test has these connections defined.
            assert self.dut.mb.node_0.soc.primary_die.mcp.channel_manager is not None
            self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel().open()
            assert self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel().is_open()

            mcp_lines = ' '.join(KngPythiaTestIF.read_and_log_lines(self,connection=self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel(), num_lines=30))
            test_result = True if ('HeartBeat' in mcp_lines) else False

            self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel().close()
            self.dut.teardown()

            # Notify of the test results. If test_result is False it is an error (which also signals to the base test class that the test failed).
            self.test_notify(step="HeartBeat", msg="Test Done", _is_error=not(test_result))
            return test_result
        
        else:
            self.log.info("Test not implemented/applicable for this DUT. Bypassing, returning PASS")
            time.sleep(2)
            return True