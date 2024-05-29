"""
- Python based Pythia 2.0 Test.
- Tests that the HeartBeat from the SCP and MCP cores transmits via the UART.
"""

from pathlib import Path
from ..test_lib.base_mscp_test import BaseMSCPTest

# Class name must match file name for Robot Framework Library usage
class mscp_heart_beat(BaseMSCPTest):

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
        name: str = "mscp_runtime_fw_heart_beat",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        host_config: Path | str = None,
    ):

        # Call parent class init
        super().__init__(
            name=name,
            workspace_config=workspace_config,
            default_log_home=default_log_home,
            fw_payload_path=fw_payload_path,
            host_config=host_config,
        )

    def test(self):
        """
        Test function:
            1. Setup the Test.
            2. Assert that a connection to SCP/MCP on DIE 0 can be
                established and open it.
            3. Look for the HeartBeat from the SCP/MCP.
            4. Teardown Test.
        """
        self.dut.setup()

        # Ensure the host config file used alongside this test has these connections defined.

        assert self.dut.mb.soc_0.scp is not None
        assert self.dut.mb.soc_0.mcp is not None

        self.dut.mb.soc_0.scp.open()
        self.dut.mb.soc_0.mcp.open()
        assert self.dut.mb.soc_0.scp.is_open()
        assert self.dut.mb.soc_0.mcp.is_open()

        scp_lines = ' '.join(self.read_and_log_lines(connection=self.dut.mb.soc_0.scp, num_lines=30))
        mcp_lines = ' '.join(self.read_and_log_lines(connection=self.dut.mb.soc_0.mcp, num_lines=30))

        test_pass = True if ('HeartBeat' in scp_lines and 'HeartBeat' in mcp_lines) else False

        self.dut.mb.soc_0.scp.close()
        self.dut.mb.soc_0.mcp.close()

        # Notify of the test results. If test_pass is False it is an error (which also signals to the base test class that the test failed).
        self.test_notify(step="HeartBeat", msg="Test Done", _is_error=not(test_pass))
        self.dut.teardown()

        return test_pass