# Copyright (c) Microsoft Corporation. All rights reserved.
"""
sensor_fifo_cli_test.py - Python based Pythia 2.0 Test.
Tests that check for scp sensor fifo cli command helper output.
"""

import sys
from pathlib import Path
from typing import Union, List, Optional

sys.path.append(str(Path(__file__).parent.parent / 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class SensorFifoCliTest(EchoFallsBaseTest):
    def __init__(
        self,
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        host_config: Path | str = None,
        name: str = None,
        number: str = "NaN",
        dut_platform: str = "KingsgateSVP",
        host_name: str | None = None,
    ):
        super().__init__(
            name=name,
            number=number,
            workspace_config=workspace_config,
            default_log_home=default_log_home,
            fw_payload_path=fw_payload_path,
            dut_platform=dut_platform,
            host_config=host_config,
            host_name=host_name,
        )
    
    def sensor_fifo_cli_test(self, command: str, num_lines: Union[str, int] = None, pass_logs: Union[List[str], str] = None, fail_logs: Union[List[str], str] = None, optional_first_command: Optional[str] = None) -> bool:
        """
        Test function to execute sensor fifo CLI commands and validate the response.
        Optionally executes a first command before the actual command.
        """
        self.log.info(f"Running SENSOR FIFO CLI TEST with command: {command}")
        self.dut.setup()

        try:
            core_com_channel = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()

            if self.dut.get_dut_type() == DeviceType.BIGFPGA:
                KngPythiaTestSetup.reset_fpga_load_prodfw(self)
            elif self.dut.get_dut_type() == DeviceType.SVP:
                core_com_channel.open()
                if not core_com_channel.is_open():
                    self.log.error("Failed to open core communication channel")
                    return False
            else:
                raise ValueError("Unsupported DUT type")

            if optional_first_command:
                self.log.info(f"Executing first command: {optional_first_command}")
                first_command_response = core_com_channel.execute_command(command=optional_first_command, command_args="")

            self.log.info(f"Submitting command: {command}")
            command_response = core_com_channel.execute_command(command=command, command_args="")

            if num_lines is not None and pass_logs is not None and fail_logs is not None:
                # Convert pass_logs and fail_logs to lists if they are strings
                if isinstance(pass_logs, str):
                    self.log.info(f"Pass log (string): {pass_logs}")  # Log pass_logs when it's a string
                    pass_logs_list = [pass_logs]
                else:
                    pass_logs_list = pass_logs

                fail_logs_list = [fail_logs] if isinstance(fail_logs, str) else fail_logs

                # Log the pass_logs_list for visibility
                self.log.info(f"Pass logs (list): {pass_logs_list}")  # Log pass_logs_list
                
                # Convert num_lines to integer if it's a string
                num_lines_int = int(num_lines) if isinstance(num_lines, str) else num_lines

                if self.dut.get_dut_type() == DeviceType.BIGFPGA:
                    test_result = KngPythiaTestIF.parse_log(self, log_lines=command_response[1], pass_logs=pass_logs_list, fail_logs=fail_logs_list)
                elif self.dut.get_dut_type() == DeviceType.SVP:
                    command_response = KngPythiaTestIF.read_from_uart(self, connection=self.dut.mb.node_0.soc.primary_die.scp.channel_manager, num_lines=num_lines_int)
                    test_result = KngPythiaTestIF.parse_log(self, log_lines=command_response, pass_logs=pass_logs_list, fail_logs=fail_logs_list)
                return test_result
            else:
                return True

        except Exception as e:
            self.log.error(f"Error during test execution: {str(e)}")
            return False
        finally:
            self.dut.teardown()