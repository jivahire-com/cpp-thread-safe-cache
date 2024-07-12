"""
- Python based Pythia 2.0 Test.
- Tests that the HeartBeat from the SCP and MCP cores for bootloader FW transmits via the UART.
"""

from pathlib import Path
from ..test_lib.base_mscp_test import BaseMSCPTest

# Class name must match file name for Robot Framework Library usage
class mscp_heart_beat_bootloader(BaseMSCPTest):

    """
    :param name: 
        Name of the test case
    :param workspace_config:
        Path to the workspace config file
    :param default_log_home:
        Path for log storage
    :param fw_payload_path:
        Path to the recipe payload
    :param dut_platform:
        Platform used during the test case execution
    :param host_config:
        Path to the host config file/directory
    """
    def __init__(
        self,
        name: str = None,
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        host_config: Path | str = None,
        test_core: str = None,
    ):

        name = test_core + "_bl_embed_fw_heart_beat"

        # Call parent class init
        super().__init__(
            name=name,
            workspace_config=workspace_config,
            default_log_home=default_log_home,
            fw_payload_path=fw_payload_path,
            host_config=host_config,
        )

        self.test_core = test_core

    def test(self):
        if self.test_core == "scp":
            return self.test_scp_bl_embed_fw()
        elif self.test_core == "mcp":
            return self.test_mcp_bl_embed_fw()
        else:
            raise ValueError(f"Unknown core: {self.test_core}")


    def test_scp_bl_embed_fw(self):
        """
        Test function:
            1. Setup the Test.
            2. Assert that a connection to SCP on DIE 0 can be
                established and open it.
            3. Look for the HeartBeat from the SCP.
            4. Teardown Test.
        """
        self.dut.setup()

        # Ensure the host config file used alongside this test has these connections defined.

        assert self.dut.mb.node_0.soc.primary_die.scp.channel_manager is not None

        self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel().open()

        assert self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel().is_open()

        scp_lines = ' '.join(self.read_and_log_lines(connection=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel(), num_lines=30))

        test_pass = True if ('HeartBeat' in scp_lines) else False

        self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel().close()

        # Notify of the test results. If test_pass is False it is an error (which also signals to the base test class that the test failed).
        self.test_notify(step="HeartBeat", msg="Test Done", _is_error=not(test_pass))
        self.dut.teardown()

        return test_pass

    def test_mcp_bl_embed_fw(self):
        """
        Test function:
            1. Setup the Test.
            2. Assert that a connection to MCP on DIE 0 can be
                established and open it.
            3. Look for the HeartBeat from the MCP.
            4. Teardown Test.
        """
        self.dut.setup()

        # Ensure the host config file used alongside this test has these connections defined.
        assert self.dut.mb.node_0.soc.primary_die.mcp.channel_manager is not None

        self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel().open()

        assert self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel().is_open()

        mcp_lines = ' '.join(self.read_and_log_lines(connection=self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel(), num_lines=30))

        test_pass = True if ('HeartBeat' in mcp_lines) else False

        self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel().close()

        # Notify of the test results. If test_pass is False it is an error (which also signals to the base test class that the test failed).
        self.test_notify(step="HeartBeat", msg="Test Done", _is_error=not(test_pass))
        self.dut.teardown()

        return test_pass
