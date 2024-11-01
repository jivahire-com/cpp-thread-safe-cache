# Copyright (c) Microsoft Corporation. All rights reserved.
"""
sensor_fifo_cli_test.py - Python based Pythia 2.0 Test.
Tests that check for sensor fifo cli command helper output.
"""

import sys, os
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
    
    def sensor_fifo_cli_test(self, command: str, read_until_key: str, pass_logs: Union[List[str], str] = None, optional_first_command: Optional[str] = None) -> bool:
        """
        Test function to execute sensor fifo CLI commands and validate the response.
        Optionally executes a first command before the actual command.
        """
        self.log.info(f"Running SENSOR FIFO CLI TEST with command: {command}")
        self.dut.setup()

        try:
            core_com_channel = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()

            if self.dut.get_dut_type() == DeviceType.BIGFPGA:
                # TODO: Need to implement without reset when testing in BIGFPGA
                KngPythiaTestSetup.reset_fpga_sideload_testfw(self.dut, self.log)
                self.log.info(f"Testing on BIGFPGA")
                return True

            elif self.dut.get_dut_type() == DeviceType.SVP:
                self.log.info(f"opening channel for SVP")
                core_com_channel.open()
                if not core_com_channel.is_open():
                    self.log.error("Failed to open core communication channel")
                    return False
                if optional_first_command:
                    self.log.info(f"Executing first command: {optional_first_command}")
                    core_com_channel.write_line(write_string=optional_first_command)

                self.log.info(f"Executing command: {command}")
                core_com_channel.write_line(write_string=command)
                
                try:
                    command_response = core_com_channel.read_until(key=read_until_key, timeout_seconds=60)
                    self.log.info("Received Response Successfully from UART . . .")
                    self.log.info(command_response)
                except Exception as e:
                    self.log.error(f"Error reading UART: {e}")
                    core_com_channel.close()
                    return False

                if pass_logs is not None:
                    # Convert pass_logs to a list if they are strings
                    if isinstance(pass_logs, str):
                        self.log.info(f"Pass log (string): {pass_logs}")  # Log pass_logs when it's a string
                        pass_logs_list = [pass_logs]
                    else:
                        pass_logs_list = pass_logs

                    # Log the pass_logs_list for visibility
                    self.log.info(f"Pass logs (list): {pass_logs_list}")  # Log pass_logs_list

                    for item in pass_logs_list:
                        if item in command_response:
                            self.log.info(f"Found: '{item}'")
                        else:
                            self.log.error(f"Not Found: '{item}'")
                            return False
                # If no pass_logs are provided in a Robot test case, assume it's a command execution test and return command_response or true to pass.    
                if command_response:
                    return command_response
                else:
                    return True                
            else:
                raise ValueError("Unsupported DUT type")

           
        except Exception as e:
            self.log.error(f"Error during test execution: {str(e)}")
            return False
        finally:
            self.log.info("Tearing DOWN...")
            self.dut.teardown()

    #Creating a separate keyword for validating test_result as the return value can be True or a string(Command response)
    def validate_test_result(self, test_result):
        if test_result is True or (isinstance(test_result, str) and test_result.strip()):
            return True
        else:
            raise AssertionError("Test result is not True or a non-empty string.")