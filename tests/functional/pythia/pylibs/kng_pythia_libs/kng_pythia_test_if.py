"""
- Python based Pythia 2.0 Test.
- This module contains interface and parsing related utility functions for pythia functional tests.
"""

import re
from pathlib import Path

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

from pythia.tdk.echofalls.constants.dut_types import DeviceType

class KngPythiaTestIF(EchoFallsBaseTest):

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

    def read_from_uart(self, connection, num_lines):
        """
        Read lines from the UART connection.

        :param connection: The UART connection object.
        :param num_lines: Number of lines to read.
        :return: List of lines read.
        """
        
        read_log = []
        for x in range(num_lines):
            try:
                line = connection.get_current_channel().read_line()
                read_log.append(line)
                self.log.info(f"Line [{x}]: \n {line}")
            except Exception as e:
                self.log.error(f"Failed to read line [{x}] of [{num_lines}] lines: {str(e)}")
        
        return read_log
    
    def parse_log(self, log_lines, pass_logs, fail_logs):
        """
        Check for failure and success strings in the provided lines, removing numbers, 
        and return a summary. The test will fail if any failure strings are found or 
        if any success strings are missing.

        :param log_lines: List of lines read from UART.
        :param pass_logs: List of success strings to look for.
        :param fail_logs: List of failure strings to look for.
        :return: A dictionary with success, failure, and missing success strings.
                Returns False if any failure string is found or any success string is missing.
        """
        print("Raw Log:", log_lines)

        result = {
            "success": [],
            "failure": [],
            "missing_success": []
        }

        # Remove numbers from success and failure strings
        pass_logs = [re.sub(r'\d+', '', success_str) for success_str in pass_logs]
        fail_logs = [re.sub(r'\d+', '', failure_str) for failure_str in fail_logs]

        # Compile a pattern for each failure string to match exact words
        fail_patterns = [re.compile(r'\b{}\b'.format(re.escape(fail_string))) for fail_string in fail_logs]

        if (self.dut.get_dut_type() == DeviceType.BIGFPGA):
            log_lines_formatted = log_lines.split("\n")
        
        else:
            log_lines_formatted = log_lines
        
        # Check for any failure string in any line
        for line in log_lines_formatted:
            # Remove numbers from the current line
            log_lines_without_numbers = re.sub(r'\d+', '', line).strip()

            # Check for failure patterns
            for fail_pattern in fail_patterns:
                if re.search(fail_pattern, log_lines_without_numbers):
                    self.log.error(f"Failure string '{fail_pattern.pattern}' found in line: {log_lines_without_numbers}")
                    result["failure"].append(fail_pattern.pattern)
                    return False  # Immediate test failure if a failure string is found

        # Track success strings found and missing
        success_found = set()
        for line in log_lines_formatted:
            # Remove numbers from the current line
            log_lines_without_numbers = re.sub(r'\d+', '', line).strip()

            # Check for success strings
            for success_str in pass_logs:
                if success_str in log_lines_without_numbers:
                    self.log.info(f"Success string '{success_str}' found in line: {log_lines_without_numbers}")
                    result["success"].append(success_str)
                    success_found.add(success_str)

        # Identify missing success strings
        for success_str in pass_logs:
            if success_str not in success_found:
                self.log.error(f"Success string '{success_str}' not found in the data.")
                result["missing_success"].append(success_str)

        # Fail the test if any success strings are missing
        if result["missing_success"]:
            return False  # Test fails if any success strings are missing

        return result
    
    def write_to_uart(self, connection, command):
        """
            Write a command to the UART connection and get the result.
            :param connection: The UART connection object.
            :param cmd: The command to execute on the UART connection.
            :return: The result of the command execution.
        """
        assert self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel().is_open()
        
        command_response = ""
        command_response = connection.get_current_channel().execute_command(command=command, command_args= "")
        return command_response
