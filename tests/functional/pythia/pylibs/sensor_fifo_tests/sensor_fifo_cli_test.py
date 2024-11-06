# Copyright (c) Microsoft Corporation. All rights reserved.
"""
sensor_fifo_cli_test.py - Python based Pythia 2.0 Test.
Tests that check for sensor fifo cli command helper output.
"""

import sys, os
from pathlib import Path
from typing import Union, List, Dict, Optional

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
            core_com_channel.close()

    #Creating a separate keyword for validating test_result as the return value can be True or a string(Command response)
    def validate_test_result(self, test_result):
        if test_result is True or (isinstance(test_result, str) and test_result.strip()):
            return True
        else:
            raise AssertionError("Test result is not True or a non-empty string.")
        
    def set_and_verify_fifo_state(self, fifo_id: int, enable: bool = True) -> bool:
        """
        Sets a specific FIFO's enable state and verifies the change.
        """
        try:
            # Get FIFO info for logging
            initial_info = self.get_structured_fifo_info(fifo_id)
            if not initial_info:
                self.log.error(f"Failed to get initial info for FIFO {fifo_id}")
                return False
                
            # Convert enable bool to command value (1 for enable, 0 for disable)
            enable_value = "1" if enable else "0"
            command = f"snsrfifo fifoen {fifo_id} {enable_value}"
            expected_state = "enabled" if enable else "disabled"
            
            self.log.info(f"Attempting to set FIFO {fifo_id} ({initial_info['name']}) to {expected_state}")
            
            # Execute the enable/disable command
            command_result = self.run_command(command, "Ok")
            if not command_result:
                self.log.error(f"Failed to execute {command}")
                return False
                
            # Get FIFO info to verify the state change
            fifo_info = self.get_structured_fifo_info(fifo_id)
            if not fifo_info:
                self.log.error(f"Failed to get FIFO {fifo_id} information after state change")
                return False
                
            # Verify the state changed as expected
            actual_state = fifo_info['enabled']
            if actual_state != enable:
                self.log.error(f"FIFO {fifo_id} ({fifo_info['name']}) state verification failed. "
                            f"Expected: {enable}, Actual: {actual_state}")
                return False
                
            self.log.info(f"Successfully {expected_state} FIFO {fifo_id} ({fifo_info['name']})")
            return True
                
        except Exception as e:
            self.log.error(f"Error setting FIFO {fifo_id} state: {str(e)}")
            return False
    

    def get_structured_fifo_info(self, fifo_id: Optional[int] = None) -> Union[Dict[int, dict], Optional[dict]]:
        """
        Gets detailed sensor FIFO information in a Robot-friendly dictionary format.
        Regarding Args:
        fifo_id: Can be either string or integer. If string, will be converted to int.
        """
        try:
            # Convert fifo_id to int if it's a string
            if isinstance(fifo_id, str) and fifo_id.isdigit():
                fifo_id = int(fifo_id)
                self.log.info(f"Converted string fifo_id to int: {fifo_id}")

            command_response = self.run_command("snsrfifo lprop", "Ok")
            self.log.info(f"Raw command response:\n{command_response}")
            
            if not command_response:
                self.log.error("Expected string response from sensor_fifo_cli_test")
                return None
                
            # Skip the command echo and 'Ok' in the response
            response_lines = [line for line in command_response.split('\n') 
                            if line and not line.startswith('snsrfifo') and not line == 'Ok']
            
            fifo_data = {}
            current_fifo_lines = []
            
            for line in response_lines:
                if 'Fifo ID =' in line:
                    if current_fifo_lines:
                        self.parse_fifo_entry(current_fifo_lines, fifo_data)
                    current_fifo_lines = []
                if line.strip():
                    current_fifo_lines.append(line)
            
            # Process the last FIFO
            if current_fifo_lines:
                self.parse_fifo_entry(current_fifo_lines, fifo_data)
            
            self.log.info(f"Successfully parsed {len(fifo_data)} FIFOs")
            
            # Return based on whether a specific FIFO ID was requested
            if fifo_id is not None:
                if fifo_id in fifo_data:
                    self.log.info(f"Returning info for FIFO {fifo_id}: {fifo_data[fifo_id]}")
                    return fifo_data[fifo_id]
                self.log.error(f"FIFO ID {fifo_id} not found. Available IDs: {sorted(fifo_data.keys())}")
                return None
            return fifo_data
                
        except Exception as e:
            self.log.error(f"Error getting sensor FIFO info: {str(e)}")
            return None

    # This method is not used but kept it for reference and to quickly check parsing logic for future changes.
    # def parse_fifo_entry(self, lines: List[str], fifo_data: dict) -> None:
    #     """Helper method to parse a single FIFO entry"""
    #     try:
    #         # Parse first line for ID and name
    #         first_line = lines[0]
    #         id_part = first_line.split('Fifo ID =')[1].split(':')[0].strip()
    #         name_part = first_line.split('Fifo Name =')[1].strip()
            
    #         current_id = int(id_part)
    #         fifo_name = name_part
            
    #         # Parse second line for sizes
    #         size_line = lines[1].strip()
    #         entry_size = int(size_line.split('entry size =')[1].split(':')[0].strip())
    #         stride_size = int(size_line.split('stride size =')[1].split(':')[0].strip())
    #         num_entries = int(size_line.split('num entries or strides =')[1].strip())
            
    #         # Parse third line for addresses
    #         addr_line = lines[2].strip()
    #         start_addr = addr_line.split('start addr =')[1].split(':')[0].strip()
    #         end_addr = addr_line.split('end addr =')[1].strip()
            
    #         # Parse fourth line for status
    #         status_line = lines[3].strip()
    #         enabled = status_line.split('enabled =')[1].split(':')[0].strip().lower() == 'true'
    #         is_empty = status_line.split('empty =')[1].strip().lower() == 'yes'
            
    #         fifo_data[current_id] = {
    #             'fifo_id': current_id,
    #             'name': fifo_name,
    #             'entry_size': entry_size,
    #             'stride_size': stride_size,
    #             'num_entries': num_entries,
    #             'start_addr': start_addr,
    #             'end_addr': end_addr,
    #             'enabled': enabled,
    #             'is_empty': is_empty
    #         }
    #         self.log.info(f"Successfully parsed FIFO {current_id}")
            
    #     except Exception as e:
    #         self.log.warning(f"Error parsing FIFO entry: {str(e)}\nLines:\n{lines}")

    def parse_fifo_entry(self, lines: List[str], fifo_data: dict) -> None:
        """Helper method to parse a single FIFO entry
        
        Args:
            lines: List of strings containing FIFO information
            fifo_data: Dictionary to store parsed FIFO data
        """
        # Check if this is a valid FIFO entry before attempting to parse
        if not lines or not any('Fifo ID =' in line for line in lines):
            return

        try:
            # Parse first line for ID and name
            first_line = lines[0]
            if 'Fifo ID =' not in first_line or 'Fifo Name =' not in first_line:
                return

            # Extract FIFO ID and name
            id_part = first_line.split('Fifo ID =')[1].split(':')[0].strip()
            name_part = first_line.split('Fifo Name =')[1].strip()
            fifo_id = int(id_part)

            # Initialize FIFO info dictionary
            fifo_info = {
                'fifo_id': fifo_id,
                'name': name_part,
                'entry_size': 0,
                'stride_size': 0,
                'num_entries': 0,
                'start_addr': '0x0',
                'end_addr': '0x0',
                'enabled': False,
                'is_empty': True
            }

            # Parse second line for sizes and entries
            if len(lines) > 1:
                size_line = lines[1]
                entry_size = int(size_line.split('entry size =')[1].split(':')[0].strip())
                stride_size = int(size_line.split('stride size =')[1].split(':')[0].strip())
                num_entries = int(size_line.split('num entries or strides =')[1].strip())
                fifo_info['entry_size'] = entry_size
                fifo_info['stride_size'] = stride_size
                fifo_info['num_entries'] = num_entries

            # Parse third line for addresses
            if len(lines) > 2:
                addr_line = lines[2]
                start_addr = addr_line.split('start addr =')[1].split(':')[0].strip()
                end_addr = addr_line.split('end addr =')[1].strip()
                fifo_info['start_addr'] = start_addr
                fifo_info['end_addr'] = end_addr

            # Parse fourth line for status
            if len(lines) > 3:
                status_line = lines[3]
                enabled = status_line.split('enabled =')[1].split(':')[0].strip().lower() == 'true'
                is_empty = status_line.split('empty =')[1].strip().lower() == 'yes'
                fifo_info['enabled'] = enabled
                fifo_info['is_empty'] = is_empty

            # Store the parsed FIFO info in the fifo_data dictionary
            fifo_data[fifo_id] = fifo_info

        except (IndexError, ValueError, AttributeError) as e:
            # Only log errors for valid FIFO entries that failed to parse
            if 'Fifo ID =' in lines[0]:
                self.log.warning(f"Error parsing FIFO entry: {str(e)}\nLines:\n{lines}")

    def setup_dut(self) -> bool:
        """Setup the DUT for testing"""
        try:
            self.log.info("Setting up DUT...")
            self.dut.setup()
            return True
        except Exception as e:
            self.log.error(f"Error during DUT setup: {str(e)}")
            return False

    def teardown_dut(self) -> bool:
        """Teardown the DUT after testing"""
        try:
            self.log.info("Tearing down DUT...")
            self.dut.teardown()
            return True
        except Exception as e:
            self.log.error(f"Error during DUT teardown: {str(e)}")
            return False
        
    def run_command(self, command: str, read_until_key: str) -> Union[str, bool]:
        """
        Execute a single command and get its output. This function doesn't handle dut setup and teardown. Added this function to handle the command execution only.
        Args:
            command (str): The command to execute
            read_until_key (str): The key to read until (Most of the cases it's 'Ok')
        Returns:
            Union[str, bool]: Command response string if successful, False if failed
        """
        self.log.info(f"Running command: {command}")

        try:
            core_com_channel = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()

            if self.dut.get_dut_type() == DeviceType.BIGFPGA:
                # TODO: Need to implement without reset when testing in BIGFPGA
                KngPythiaTestSetup.reset_fpga_sideload_testfw(self.dut, self.log)
                self.log.info(f"Testing on BIGFPGA")
                return True

            elif self.dut.get_dut_type() == DeviceType.SVP:
                self.log.info(f"Opening channel for SVP")
                core_com_channel.open()
                if not core_com_channel.is_open():
                    self.log.error("Failed to open core communication channel")
                    return False

                self.log.info(f"Executing command: {command}")
                core_com_channel.write_line(write_string=command)
                
                try:
                    command_response = core_com_channel.read_until(key=read_until_key, timeout_seconds=60)
                    self.log.info("Received Response Successfully from UART . . .")
                    # self.log.info(command_response)
                    return command_response
                except Exception as e:
                    self.log.error(f"Error reading UART: {e}")
                    return False
                finally:
                    core_com_channel.close()
            else:
                raise ValueError("Unsupported DUT type")
            
        except Exception as e:
            self.log.error(f"Error during command execution: {str(e)}")
            return False