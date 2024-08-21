# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests that checks for sdm memcpy completes from AP core0.
"""

import re
from pathlib import Path
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class sdm_pcie_tests(EchoFallsBaseTest):

    """
    :param name: 
        Name of the test case
    :param workspace_config:
        Path to the workspace config file
    :param default_log_home:
        Path for log storage
    :param fw_payload_path:
        Path to the recipe payload
    :param host_config:
        Path to the host config file/directory
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

    def read_from_uart(self, connection, num_lines):
        """
        Read lines from the UART connection.

        :param connection: The UART connection object.
        :param num_lines: Number of lines to read.
        :return: List of lines read.
        """
        
        lines = []
        for x in range(num_lines):
            try:
                line = connection.get_current_channel().read_line()
                lines.append(line)
                self.log.info(f"Line [{x}]: \n {line}")
            except Exception as e:
                self.log.error(f"Failed to read line [{x}] of [{num_lines}] lines: {str(e)}")
        
        return lines

    def write_to_uart(self, connection, cmd):
        """
            Write a command to the UART connection and get the result.
            :param connection: The UART connection object.
            :param cmd: The command to execute on the UART connection.
            :return: The result of the command execution.
        """
        cmd_res = ""
        cmd_res = connection.get_current_channel().execute_command(command=cmd, command_args= "")
        return cmd_res

    def test_setup(self):
        """
        Test setup function:
        1. Setup the Test.
        2. Assert that a connection to cded/sdm on DIE 0 can be established and open it.
        3. Ensure the connection is successfully opened.
        """

        self.dut.setup()    
        assert self.dut.mb.node_0.soc.primary_die.apns.channel_manager is not None
        self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel().open()
        self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel().is_open()


    def strings_to_look(self, dut_lines_data, success_strings, failure_strings):
        """
        Check for failure and success strings in the provided lines, ignoring numbers in the data.

        :param dut_lines_data: List of lines read from UART.
        :param success_strings: List of success strings to look for.
        :param failure_strings: List of failure strings to look for.
        :return: True if all success strings are found as full lines and failure strings are absent.
        """
        print("dut lines data:", dut_lines_data)

        # Compile a pattern for each failure string to match exact words
        failure_patterns = [re.compile(r'\b{}\b'.format(re.escape(fail_str))) for fail_str in failure_strings]

        # Check for any failure string in any line
        for line in dut_lines_data:
            for fail_pattern in failure_patterns:
                if re.search(fail_pattern, line):
                    self.log.error(f"Failure string '{fail_pattern.pattern}' found in line: {line.strip()}")
                    return False  # Immediate return on failure detection

        # Clean and preprocess the lines by removing or replacing numbers
        modified_dut_lines_data = []
        for line in dut_lines_data:
            # Remove numbers
            modified_line = re.sub(r'\d+', '', line).strip()
            modified_dut_lines_data.append(modified_line)

        # Join all modified lines into a single text
        full_text = ' '.join(modified_dut_lines_data)

        # Check for the presence of each success string in the modified text
        for success_str in success_strings:
            if success_str not in full_text:
                self.log.error(f"Success string '{success_str}' not found in data.")
                return False
            else:
                self.log.info(f"Success string '{success_str}' found.")

        # Return True if all success strings are found and no failures are detected
        return True


    def test_teardown(self):
        """
        Test teardown function:
            1. Close the connection.
            2. Teardown the DUT.
        """
        if self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel().is_open():
            self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel().close()
        self.dut.teardown()