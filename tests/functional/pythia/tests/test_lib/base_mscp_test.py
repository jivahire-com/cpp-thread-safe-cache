"""
- Python based Pythia 2.0 Test.
- Tests that the HeartBeat from the SCP and MCP cores transmits via the UART.
"""

from pathlib import Path
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class BaseMSCPTest(EchoFallsBaseTest):

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
    
    def read_and_log_lines(self, connection, num_lines):
        self.log.info(f"Reading [{num_lines}] lines from [{connection}]")

        lines = []
        for x in range(0, num_lines):
            try:
                line = connection.read_line()
                lines.append(line)
                self.log.info(f"Line [{x}]: \n {line}")
            except:
                self.log.error(f"Failed to read line [{x}] of [{num_lines}] lines")
        return lines

    def test(self):
        raise NotImplementedError("Abstract method test must be overridden")
