"""
- Python based Pythia 2.0 Test.
- Tests that the HeartBeat from the SCP and MCP cores transmits via the UART.
"""

import argparse
import time

from pathlib import Path
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class heart_beat(EchoFallsBaseTest):

    """
    :param name: 
        Name of the test case
    :param number:
        ADO Number of the test case
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
    :param host_name:
        Name of the host to find the host config file, if host_config is a directory. Defaults to None
    """
    def __init__(
        self,
        name: str = "MSCP_Heart_Beat",
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

    def read_and_log_lines(self, connection, num_lines):
        self.log.info(f"Reading [{num_lines}] lines from [{connection}]")

        lines = []
        for x in range(0,num_lines):
            try:
                line = connection.read_line()
                lines.append(line)
                self.log.info(f"Line [{x}]: \n {line}")
            except:
                self.log.error(f"Failed to read line [{x}] of [{num_lines}] lines")
        return lines

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


if __name__ == "__main__":

    # Create an ArgumentParser object
    parser = argparse.ArgumentParser(description="Process some named parameters.")

    # Add arguments to the ArgumentParser object
    parser.add_argument("--workspace_config", type=str, help="Workspace config JSON.")
    parser.add_argument("--log_dir", type=str, help="Logs directory.")
    parser.add_argument("--payload_dir", type=str, help="Payload Directory.")
    parser.add_argument("--host_config", type=str, help="Host config JSON.")

    # Parse the command-line arguments
    args = parser.parse_args()

    test = heart_beat(
        workspace_config=args.workspace_config,
        default_log_home=args.log_dir,
        fw_payload_path=args.payload_dir,
        host_config=args.host_config
    )
    assert test.test()
