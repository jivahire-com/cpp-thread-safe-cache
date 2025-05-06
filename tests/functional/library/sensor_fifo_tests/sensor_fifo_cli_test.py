# Copyright (c) Microsoft Corporation. All rights reserved.
"""
sensor_fifo_cli_test.py - Python based Pythia 2.0 Test.
Tests that check for sensor fifo cli command helper output.
"""
import time
import sys, os
import time
from pathlib import Path
from typing import Union, List, Dict, Optional

sys.path.append(str(Path(__file__).parent.parent / 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend([
    pylibs_dir,
    current_dir,
])

try:
    # Try relative import first
    from .sensor_data_generator import SensorDataGenerator
except ImportError:
    # Fall back to direct import
    from sensor_data_generator import SensorDataGenerator


class SensorFifoCliTest(EchoFallsBaseTest):
    # Initialize class-level flag
    _scp_mcp_heartbeat_received = False

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
        self.core_com_channel_scp = None
        self.core_com_channel_mcp = None

    def wait_for_scp_mcp_heartbeat(self) -> bool:
        """Wait for both SCP and MCP heartbeat messages"""
        try:
            # Open channels
            self.core_com_channel_scp.open()
            self.core_com_channel_mcp.open()
            
            # Wait for SCP heartbeat
            self.log.info("Waiting for initial SCP Heartbeat Message...")
            try:
                self.core_com_channel_scp.read_until(key="ScpHeartBeat", timeout_seconds=1800)
                self.log.info("SCP Heartbeat received successfully")
            except Exception as e:
                self.log.error(f"Failed to receive SCP heartbeat: {str(e)}")
                return False
                
            # Wait for MCP heartbeat
            self.log.info("Waiting for initial MCP Heartbeat Message...")
            try:
                self.core_com_channel_mcp.read_until(key="McpHeartBeat", timeout_seconds=1800)
                self.log.info("MCP Heartbeat received successfully")
            except Exception as e:
                self.log.error(f"Failed to receive MCP heartbeat: {str(e)}")
                return False
                
            self.__class__._scp_mcp_heartbeat_received = True
            return True
            
        except Exception as e:
            self.log.error(f"Error during heartbeat check: {str(e)}")
            return False
        finally:
            # Close channels
            if self.core_com_channel_scp.is_open():
                self.core_com_channel_scp.close()
            if self.core_com_channel_mcp.is_open():
                self.core_com_channel_mcp.close()    

    def sensor_fifo_cli_test(self, command: str, read_until_key: str, pass_logs: Union[List[str], str] = None, optional_first_command: Optional[str] = None) -> bool:
        """
        Test function to execute sensor fifo CLI commands and validate the response.
        Optionally executes a first command before the actual command.
        """
        self.log.info(f"Running SENSOR FIFO CLI TEST with command: {command}")
        
        try:
            if self.dut.get_dut_type() in [DeviceType.BIGFPGA,DeviceType.SVP]:
                self.log.info(f"Testing and opening channel for {self.dut.get_dut_type().value}")
                self.core_com_channel_scp.open()
                if not self.core_com_channel_scp.is_open():
                    self.log.error("Failed to open core communication channel")
                    return False

                if optional_first_command:
                    self.log.info(f"Executing first command: {optional_first_command}")
                    self.core_com_channel_scp.write_line(write_string=optional_first_command)
                self.log.info(f"Executing command: {command}")
                self.core_com_channel_scp.write_line(write_string=command)
                try:
                    command_response = self.core_com_channel_scp.read_until(key=read_until_key, timeout_seconds=60)
                    self.log.info("Received Response Successfully from UART . . .")
                    self.log.info(command_response)
                except Exception as e:
                    self.log.error(f"Error reading UART: {e}")
                    self.core_com_channel_scp.close()
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
            self.core_com_channel_scp.close()

    def run_command_on_mcp(self, command: str, read_until_key: str, pass_logs: Union[List[str], str] = None) -> bool:
        """
        Execute a command on the MCP and validate the response.
        """
        self.log.info(f"Running command on MCP with command: {command}")
        
        try:
            self.core_com_channel_mcp.open()
            if not self.core_com_channel_mcp.is_open():
                self.log.error("Failed to open MCP communication channel")
                return False

            self.log.info(f"Executing command: {command}")
            self.core_com_channel_mcp.write_line(write_string=command)
            
            try:
                command_response = self.core_com_channel_mcp.read_until(key=read_until_key, timeout_seconds=60)
                self.log.info("Received Response Successfully from MCP . . .")
                self.log.info(command_response)
            except Exception as e:
                self.log.error(f"Error reading MCP: {e}")
                return False

            if pass_logs is not None:
                # Convert pass_logs to a list if they are strings
                if isinstance(pass_logs, str):
                    self.log.info(f"Pass log (string): {pass_logs}")
                    pass_logs_list = [pass_logs]
                else:
                    pass_logs_list = pass_logs

                for item in pass_logs_list:
                    if item in command_response:
                        self.log.info(f"Found: '{item}'")
                    else:
                        self.log.error(f"Not Found: '{item}'")
                        return False

            return command_response is not None

        except Exception as e:
            self.log.error(f"Error during MCP command execution: {str(e)}")
            return False
        finally:
            self.core_com_channel_mcp.close()

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
            command_result = self.run_command_on_scp(command, "Ok")
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

            command_response = self.run_command_on_scp("snsrfifo lprop", "Ok")
            # self.log.info(f"Raw command response:\n{command_response}")
            
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
            if self.dut.get_dut_type() == DeviceType.BIGFPGA:
                self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
                KngPythiaTestSetup.fpga_oob_reset(self.log)
                
            if self.dut.mb.node_0.soc.secondary_die is not None:
                self.log.info("Current Test is executing on DualDie Config, so secondary die will be used to open channel on SCP and MCP core")
                self.core_com_channel_scp = self.dut.mb.node_0.soc.secondary_die.scp.channel_manager.get_current_channel()
                self.core_com_channel_mcp = self.dut.mb.node_0.soc.secondary_die.mcp.channel_manager.get_current_channel()
            else:
                self.log.info("Current Test is executing on SingleDie Config, so primary die will be used to open channel on SCP and MCP core")
                self.core_com_channel_scp = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
                self.core_com_channel_mcp = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
            # Wait for SCP heartbeat during initial setup
            if not self.wait_for_scp_mcp_heartbeat():
                self.log.error("Failed to receive initial SCP-MCP heartbeat during setup")
                return False
            return True
        except Exception as e:
            self.log.error(f"Error during DUT setup: {str(e)}")
            return False

    def teardown_dut(self) -> bool:
        """Teardown the DUT after testing"""
        try:
            self.log.info("Tearing down DUT...")

            # Reset the heartbeat flag since SVP is being terminated
            self.__class__._scp_mcp_heartbeat_received = False

            self.dut.teardown()
            time.sleep(30)
            return True
        
        except Exception as e:
            self.log.error(f"Error during DUT teardown: {str(e)}")
            return False
        
    def run_command_on_scp(self, command: str, read_until_key: str) -> Union[str, bool]:
        """
        Execute a single command on scp and get its output. This function doesn't handle dut setup and teardown. Added this function to handle the command execution only.
        Args:
            command (str): The command to execute
            read_until_key (str): The key to read until (Most of the cases it's 'Ok')
        Returns:
            Union[str, bool]: Command response string if successful, False if failed
        """
        self.log.info(f"Running command: {command}")

        try:
            if self.dut.get_dut_type() in [DeviceType.BIGFPGA,DeviceType.SVP]:
                self.log.info(f"Testing and Opening channel for {self.dut.get_dut_type().value}")
                self.core_com_channel_scp.open()
                if not self.core_com_channel_scp.is_open():
                    self.log.error("Failed to open core communication channel")
                    return False

                self.log.info(f"Executing command: {command}")
                self.core_com_channel_scp.write_line(write_string=command)
                
                try:
                    command_response = self.core_com_channel_scp.read_until(key=read_until_key, timeout_seconds=60)
                    self.log.info("Received Response Successfully from UART . . .")
                    # self.log.info(command_response)
                    return command_response
                except Exception as e:
                    self.log.error(f"Error reading UART: {e}")
                    return False
                finally:
                    self.core_com_channel_scp.close()
            else:
                raise ValueError("Unsupported DUT type")
            
        except Exception as e:
            self.log.error(f"Error during command execution: {str(e)}")
            return False

    def setup_fifo_prerequisites(self) -> bool:
        """
        Sets up prerequisites required for FIFO testing.
        Should be called once before testing any FIFOs.
        
        Returns:
            bool: True if prerequisites were set successfully, False otherwise
        """

        try:
            self.log.info("Setting up prerequisites for FIFO testing")

            # Wait for SCP heartbeat
            if not self.wait_for_scp_mcp_heartbeat():
                self.log.error("Failed to receive SCP MCP heartbeat")
                return False
        
            # Wait for SVP initialization
            time.sleep(5)  # Short delay for SVP to settle

            # Ensure UART is ready by checking channel
            if not self.core_com_channel_scp.is_open():
                self.log.info("Opening UART channel")
                self.core_com_channel_scp.open()
                if not self.core_com_channel_scp.is_open():
                    self.log.error("Failed to open UART channel")
                    return False

            # Set power loop disable
            pwr_loop_result = self.sensor_fifo_cli_test(
                command="pwr set loopdis 7",
                read_until_key="Ok",
                pass_logs="pwr_cli_comp"
            )
            if not self.validate_test_result(pwr_loop_result):
                self.log.error("❌ Failed to set power loop disable")
                return False
            
            # pwrtlm disable on MCP <MCP not getting loaded when sideloaded...
            # In ADO might not work but have the code as it might be needed when we have to test in real hardware. 
            pwrtlm_result = self.run_command_on_mcp(
                command="pwrtlm disable",
                read_until_key="Ok",
                pass_logs="Power Telemetry Disabled!"
            )
            if not self.validate_test_result(pwrtlm_result):
                self.log.error("❌ Failed to disable power telemetry on MCP")
                return False

            # Reset all FIFOs
            reset_result = self.sensor_fifo_cli_test(
                command="snsrfifo reset",
                read_until_key="Ok",
                pass_logs="Fifos have been reset"
            )
            time.sleep(5)
            
            if not self.validate_test_result(reset_result):
                self.log.error("❌ Failed to reset FIFOs")
                return False

            self.log.info("✅ FIFO prerequisites set successfully")
            return True

        except Exception as e:
            self.log.error(f"❌ Error setting up FIFO prerequisites: {str(e)}")
            return False


    # With improved error handling
    def test_fifo_write_read_entries_with_improved_error_handling(self, fifo_name: str = "TEMPERATURE", random_seed: int = None, num_entries: int = 10) -> tuple[bool, list]:
        """
        Test case to validate FIFO write and read entry functionality.
        
        Args:
            fifo_name: Name of the FIFO to test (defaults to "TEMPERATURE")
                    Valid options: PSTATE, SCP_MSG, TEMPERATURE, VOLTAGE, CURRENT,
                                PVT_TEMP, PVT_VOLTAGE, DIMM_TEMP, VR_TEMP, VR_CURRENT
            random_seed: Optional seed for data generation
            num_entries: Number of entries to write and read (default: 10)
        
        Returns:
            tuple[bool, list]: (overall_status, detailed_results)
                - overall_status: True if all operations succeeded
                - detailed_results: List of tuples (entry_num, status, written_data, read_data)
        """
        test_passed = True
        detailed_results = []
        
        try:
            # Initialize data generator and get FIFO ID
            generator = SensorDataGenerator(seed=random_seed)
            if fifo_name not in generator.FIFO_NAME_TO_ID:
                self.log.error(f"[{fifo_name}] Invalid FIFO name")
                return False, []
                
            fifo_id = generator.FIFO_NAME_TO_ID[fifo_name].value
            
            # Step 1: Get initial FIFO info
            self.log.info(f"[{fifo_name}] Step 1: Getting FIFO properties")
            initial_info = self.get_structured_fifo_info(fifo_id)
            if not initial_info:
                self.log.error(f"[{fifo_name}] Failed to get initial info for FIFO {fifo_id}")
                return False, []
                
            entry_size = initial_info['entry_size']
            stride_size = initial_info['stride_size']
            
            self.log.info(f"[{fifo_name}] Testing {initial_info['name']} (ID: {fifo_id})")
            
            # Step 2: Enable FIFO
            self.log.info(f"[{fifo_name}] Step 2: Enabling FIFO")
            enable_result = self.sensor_fifo_cli_test(
                command=f"snsrfifo fifoen {fifo_id} 1",
                read_until_key="Ok",
                pass_logs=f"Fifo {fifo_id} Enable set to enable"
            )
            if not self.validate_test_result(enable_result):
                self.log.error(f"❌ [{fifo_name}] FIFO Enable Failed")
                return False, []

            # Generate test data
            test_data = generator.generate_sensor_fifo_data(fifo_name, num_entries=num_entries)
            if not test_data or len(test_data) < num_entries:
                self.log.error(f"[{fifo_name}] Failed to generate {num_entries} test entries")
                return False, []

            # Step 3: Write entries
            self.log.info(f"[{fifo_name}] Step 3: Writing {num_entries} entries to FIFO")
            written_values = []
            for i, entry in enumerate(test_data):
                entry_hex = " ".join(entry)
                write_result = self.sensor_fifo_cli_test(
                    command=f"snsrfifo wrentry {fifo_id} 0 {entry_hex}",
                    read_until_key="Ok",
                    pass_logs="Entry written to Fifo"
                )
                if not self.validate_test_result(write_result):
                    self.log.error(f"❌ [{fifo_name}] Write Entry {i} Failed")
                    detailed_results.append((i, False, entry, "Write Failed"))
                    return False, detailed_results
                written_values.append(entry)

            # Step 4: Update stride if needed
            if entry_size != stride_size:
                self.log.info(f"[{fifo_name}] Step 4: Updating stride (entry_size != stride_size)")
                stride_result = self.sensor_fifo_cli_test(
                    command=f"snsrfifo updstride {fifo_id}",
                    read_until_key="Ok"
                )
                if not self.validate_test_result(stride_result):
                    self.log.error(f"❌ [{fifo_name}] Stride Update Failed")
                    return False, detailed_results

            # Step 5: Read and verify entries
            self.log.info(f"[{fifo_name}] Step 5: Reading and verifying {num_entries} entries")
            for i in range(num_entries):
                read_result = self.sensor_fifo_cli_test(
                    command=f"snsrfifo rdentry {fifo_id} 0",
                    read_until_key="Ok"
                )
                if not self.validate_test_result(read_result):
                    self.log.error(f"❌ [{fifo_name}] Read Entry {i} Failed")
                    detailed_results.append((i, False, written_values[i], "Read Failed"))
                    return False, detailed_results
                
                # Extract read values
                read_values = []
                read_lines = read_result.strip().split('\n')
                for line in read_lines:
                    if line.startswith('Buffer['):
                        hex_value = line.split('=')[1].strip()
                        if hex_value.startswith('0x'):
                            read_values.append(hex_value)
                
                # Compare written vs read values
                entry_matches = set(written_values[i]) == set(read_values)
                detailed_results.append((i, entry_matches, written_values[i], read_values))
                
                if not entry_matches:
                    self.log.error(f"❌ [{fifo_name}] Entry {i} Mismatch")
                    self.log.error(f"[{fifo_name}] Written: {written_values[i]}")
                    self.log.error(f"[{fifo_name}] Read   : {read_values}")
                    test_passed = False
                    break

            # Step 6: Disable FIFO
            self.log.info(f"[{fifo_name}] Step 6: Disabling FIFO")
            disable_result = self.sensor_fifo_cli_test(
                command=f"snsrfifo fifoen {fifo_id} 0",
                read_until_key="Ok",
                pass_logs=f"Fifo {fifo_id} Enable set to disable"
            )
            if not self.validate_test_result(disable_result):
                self.log.error(f"❌ [{fifo_name}] FIFO Disable Failed")
                return False, detailed_results
            
            # Step 7: Verify write to disabled FIFO fails with expected error
            self.log.info(f"[{fifo_name}] Step 7: Verifying write to disabled FIFO fails as expected")
            entry_hex = " ".join(test_data[0])  # Use the first entry from our test data
            write_result = self.sensor_fifo_cli_test(
                command=f"snsrfifo wrentry {fifo_id} 0 {entry_hex}",
                read_until_key="ERROR",
                pass_logs="ERROR:: sensor_fifo_driver_write_entries returned 0x80000005"
            )
            
            # Check if we got the expected error for step 7
            if not self.validate_test_result(write_result):
                self.log.error(f"❌ [{fifo_name}] Failed to get expected error when writing to disabled FIFO")
                test_passed = False
            else:
                self.log.info(f"✅ [{fifo_name}] Successfully verified write to disabled FIFO fails as expected")

            # Step 8: Verify read from disabled FIFO shows empty
            self.log.info(f"[{fifo_name}] Step 8: Verifying read from disabled FIFO shows empty")
            read_result = self.sensor_fifo_cli_test(
                command=f"snsrfifo rdentry {fifo_id} 0",
                read_until_key="Ok",
                pass_logs="Fifo is empty!"
            )
            
            # Check if we got the expected empty message for step 8
            if not self.validate_test_result(read_result):
                self.log.error(f"❌ [{fifo_name}] Failed to get expected empty message when reading from disabled FIFO")
                test_passed = False
            else:
                self.log.info(f"✅ [{fifo_name}] Successfully verified read from disabled FIFO shows empty")


            if test_passed:
                self.log.info(f"✅ [{fifo_name}] write/read test completed successfully")
            else:
                self.log.error(f"❌ [{fifo_name}] write/read test completed with failures")
            
            return test_passed, detailed_results

        except Exception as e:
            self.log.error(f"❌ [{fifo_name}] Error during FIFO test: {str(e)}")
            return False, []

    
    def verify_read_data(self, read_result: str, expected_data: str) -> bool:
        """Verify that the read data matches the expected data.
        
        Args:
            read_result: The raw output from the read command
            expected_data: The expected hex values as space-separated string
        
        Returns:
            bool: True if read data matches expected data
        """
        try:
            # Extract hex values from read result
            read_values = set()
            read_lines = read_result.strip().split('\n')
            for line in read_lines:
                if line.startswith('Buffer['):
                    hex_value = line.split('=')[1].strip()
                    if hex_value.startswith('0x'):
                        read_values.add(hex_value)
            
            # Convert expected data to set of hex values
            expected_values = set(expected_data.split())
            
            # Compare sets
            if read_values == expected_values:
                return True
            else:
                self.log.error(f"Read values: {sorted(read_values)}")
                self.log.error(f"Expected values: {sorted(expected_values)}")
                return False
                
        except Exception as e:
            self.log.error(f"Error verifying read data: {str(e)}")
            return False

    
    def test_fifo_overflow(self, fifo_name: str = "TEMPERATURE", random_seed: int = None) -> bool:
        """
        Test case to validate FIFO overflow functionality based on entry and stride sizes.
        For entry_size == stride_size:
            - Writes num_entries entries, where the last write causes overflow
            - Verifies entries 2 through num_entries (last entry overwrites first)
        For entry_size != stride_size:
            - Writes num_entries × entries_per_stride entries
            - Last stride write causes overflow of first stride
            - Verifies entries from stride 2 through num_entries
        
        Args:
            fifo_name: Name of the FIFO to test (defaults to "TEMPERATURE")
            random_seed: Optional seed for random data generation
        
        Returns:
            bool: True if overflow test succeeded, False otherwise
        """
        try:
            # Initialize data generator
            data_generator = SensorDataGenerator(seed=random_seed)
            if fifo_name not in data_generator.FIFO_NAME_TO_ID:
                self.log.error(f"[{fifo_name}] Invalid FIFO name")
                return False
                
            fifo_id = data_generator.FIFO_NAME_TO_ID[fifo_name].value
            
            # Step 1: Get initial FIFO info
            self.log.info(f"[{fifo_name}] Step 1: Getting FIFO properties")
            initial_info = self.get_structured_fifo_info(fifo_id)
            if not initial_info:
                self.log.error(f"[{fifo_name}] Failed to get initial info for FIFO {fifo_id}")
                return False
                
            entry_size = initial_info['entry_size']
            stride_size = initial_info['stride_size']
            num_entries = initial_info['num_entries']

            # Step 2: Enable FIFO
            self.log.info(f"[{fifo_name}] Step 2: Enabling FIFO")
            enable_result = self.sensor_fifo_cli_test(
                command=f"snsrfifo fifoen {fifo_id} 1",
                read_until_key="Ok",
                pass_logs=f"Fifo {fifo_id} Enable set to enable"
            )
            if not self.validate_test_result(enable_result):
                self.log.error(f"❌ [{fifo_name}] FIFO Enable Failed")
                return False

            if entry_size == stride_size:
                # Generate exactly num_entries (last write will cause overflow)
                test_entries = data_generator.generate_sensor_fifo_data(fifo_name, num_entries)
                
                self.log.info(f"[{fifo_name}] Generated {len(test_entries)} test entries for equal entry/stride size FIFO:\n"
                            f"    FIFO ID = {fifo_id}: FIFO Name = {fifo_name}\n"
                            f"    Entry Size = {entry_size}: Stride Size = {stride_size}\n"
                            f"    Num Entries = {num_entries}\n"
                            f"    Start Addr = 0x{int(initial_info['start_addr'], 16):x}: End Addr = 0x{int(initial_info['end_addr'], 16):x}\n"
                            f"    Enabled = {initial_info['enabled']}")
                
                # Write entries until FIFO is almost full (num_entries - 1)
                for i, entry in enumerate(test_entries[:-1]):
                    self.log.info(f"[{fifo_name}] Writing entry {i+1}/{num_entries} to index 0")
                    write_result = self.sensor_fifo_cli_test(
                        command=f"snsrfifo wrentry {fifo_id} 0 {' '.join(entry)}",
                        read_until_key="Ok",
                        pass_logs="Entry written to Fifo"
                    )
                    if not self.validate_test_result(write_result):
                        self.log.error(f"❌ [{fifo_name}] Write Entry {i+1} Failed")
                        return False
                
                # Write the last entry which will cause overflow by overwriting first entry
                self.log.info(f"[{fifo_name}] Writing final entry (entry {num_entries}, will overflow)")
                write_result = self.sensor_fifo_cli_test(
                    command=f"snsrfifo wrentry {fifo_id} 0 {' '.join(test_entries[-1])}",
                    read_until_key="Ok",
                    pass_logs="Entry written to Fifo"
                )
                if not self.validate_test_result(write_result):
                    self.log.error(f"❌ [{fifo_name}] Final Write Entry Failed")
                    return False

            else:
                # For different sizes, multiple entries fit in one stride
                num_strides = num_entries  # Total number of strides equals num_entries when entry_size != stride_size
                entries_per_stride = stride_size // entry_size
                
                # Generate exactly num_strides × entries_per_stride entries
                total_entries = entries_per_stride * num_strides
                test_entries = data_generator.generate_sensor_fifo_data(fifo_name, total_entries)
                
                self.log.info(f"[{fifo_name}] Generated {len(test_entries)} test entries for stride-based FIFO:\n"
                            f"    FIFO ID = {fifo_id}: FIFO Name = {fifo_name}\n"
                            f"    Entry Size = {entry_size}: Stride Size = {stride_size}\n"
                            f"    Num Strides = {num_strides}\n"
                            f"    Entries per Stride = {entries_per_stride}\n"
                            f"    Total Entries = {total_entries}\n"
                            f"    Start Addr = 0x{int(initial_info['start_addr'], 16):x}: End Addr = 0x{int(initial_info['end_addr'], 16):x}\n"
                            f"    Enabled = {initial_info['enabled']}")

                # Write entries stride by stride
                entry_index = 0
                for stride in range(num_strides):  # Write all strides (last stride causes overflow)
                    # Fill one complete stride
                    for index in range(entries_per_stride):
                        self.log.info(f"[{fifo_name}] Writing entry {entry_index+1}/{total_entries} "
                                    f"to index {index} (Stride {stride+1}/{num_strides})")
                        write_result = self.sensor_fifo_cli_test(
                            command=f"snsrfifo wrentry {fifo_id} {index} {' '.join(test_entries[entry_index])}",
                            read_until_key="Ok",
                            pass_logs="Entry written to Fifo"
                        )
                        if not self.validate_test_result(write_result):
                            self.log.error(f"❌ [{fifo_name}] Write Entry Failed at index {index}, stride {stride+1}")
                            return False
                        entry_index += 1

                    # Update stride after filling all indexes
                    if stride == num_strides - 1:
                        self.log.info(f"[{fifo_name}] Updating final stride {stride+1} - first stride will be overwritten")
                    else:
                        self.log.info(f"[{fifo_name}] Updating stride {stride+1}")
                        
                    update_stride_result = self.sensor_fifo_cli_test(
                        command=f"snsrfifo updstride {fifo_id}",
                        read_until_key="Ok"
                    )
                    if not self.validate_test_result(update_stride_result):
                        self.log.error(f"❌ [{fifo_name}] Failed to update stride {stride+1}")
                        return False
            # Common verification logic after overflow
            self.log.info(f"[{fifo_name}] Reading and verifying all entries after overflow")
            success = True

            if entry_size == stride_size:
                entries_to_verify = num_entries - 1  # Skip first entry (overwritten)
                start_index = 1  # Start from second entry
                entries_per_stride = 1
            else:
                entries_to_verify = entries_per_stride * (num_strides - 1)  # Skip first stride (overwritten)
                start_index = entries_per_stride  # Start from second stride
                
                self.log.info(f"[{fifo_name}] Starting verification:\n"
                            f"    Total entries to verify: {entries_to_verify}\n"
                            f"    Starting from index: {start_index}\n"
                            f"    Entry size: {entry_size}\n"
                            f"    Stride size: {stride_size}\n"
                            f"    Entries per stride: {entries_per_stride}\n"
                            f"    Total strides: {num_strides}\n"
                            f"    Verifying strides: 2 to {num_strides}")

            # Read and verify entries sequentially
            for verification_count in range(entries_to_verify):
                # Always read from index 0
                read_result = self.sensor_fifo_cli_test(
                    command=f"snsrfifo rdentry {fifo_id} 0",
                    read_until_key="Ok"
                )
                if not self.validate_test_result(read_result):
                    self.log.error(f"❌ [{fifo_name}] Read Entry Failed for verification {verification_count + 1}")
                    return False

                # Calculate expected entry index based on start_index
                expected_entry_index = start_index + verification_count
                expected_data = ' '.join(test_entries[expected_entry_index])

                # Log verification info based on entry/stride size relationship
                if entry_size == stride_size:
                    self.log.info(f"[{fifo_name}] Verifying entry {verification_count + 1}:\n"
                                f"    Expected index: {expected_entry_index}\n"
                                f"    Expected data: {expected_data}")
                else:
                    current_stride = ((verification_count + entries_per_stride) // entries_per_stride) + 1
                    current_entry = (verification_count % entries_per_stride)
                    self.log.info(f"[{fifo_name}] Verifying entry {verification_count + 1}:\n"
                                f"    Stride {current_stride}, Entry {current_entry}\n"
                                f"    Expected index: {expected_entry_index}\n"
                                f"    Expected data: {expected_data}")
                    
                if self.verify_read_data(read_result, expected_data):
                    self.log.info(f"✅ [{fifo_name}] Verification {verification_count + 1} successful")
                else:
                    self.log.error(f"❌ [{fifo_name}] Verification {verification_count + 1} failed:\n"
                                f"    Expected: {expected_data}\n"
                                f"    Actual: {read_result}")
                    success = False
                    break

            if success:
                self.log.info(f"✅ [{fifo_name}] Overflow test succeeded - all entries verified")
            else:
                self.log.error(f"❌ [{fifo_name}] Overflow test failed - entries did not match expected values")
                return False

            # Step 3: Disable FIFO
            self.log.info(f"[{fifo_name}] Disabling FIFO")
            disable_result = self.sensor_fifo_cli_test(
                command=f"snsrfifo fifoen {fifo_id} 0",
                read_until_key="Ok",
                pass_logs=f"Fifo {fifo_id} Enable set to disable"
            )
            if not self.validate_test_result(disable_result):
                self.log.error(f"❌ [{fifo_name}] FIFO Disable Failed")
                return False

            return True

        except Exception as e:
            self.log.error(f"❌ [{fifo_name}] Error during FIFO overflow test: {str(e)}")
            return False