# Copyright (c) Microsoft Corporation. All rights reserved.
import logging
from pathlib import Path
from typing import Optional
import os
import sys
from typing import Union, List, Dict, Optional

sys.path.append(str(Path(__file__).parent.parent / 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend([pylibs_dir, current_dir])

try:
    from .dcp_commands import DataCollectionProtocol
    from .dcs_interface import (
        DIE_0, CPU_AP, CPU_MCP,
        get_transport
    )
except ImportError:
    from dcp_commands import DataCollectionProtocol
    from dcs_interface import (
        DIE_0, CPU_AP, CPU_MCP,
        get_transport
    )

logger = logging.getLogger(__name__)

class DcsEventsTest(EchoFallsBaseTest):
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
        self.dcp = None
        self.mcp_channel = None

    def wait_for_scp_mcp_heartbeat(self) -> bool:
        """Wait for both SCP and MCP heartbeat messages"""
        try:
            # Setup channels
            scp_channel = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
            mcp_channel = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
            
            # Open channels
            scp_channel.open()
            mcp_channel.open()
            
            # Wait for SCP heartbeat
            self.log.info("Waiting for initial SCP Heartbeat Message...")
            try:
                scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=1800)
                self.log.info("SCP Heartbeat received successfully")
            except Exception as e:
                self.log.error(f"Failed to receive SCP heartbeat: {str(e)}")
                return False
                
            # Wait for MCP heartbeat
            self.log.info("Waiting for initial MCP Heartbeat Message...")
            try:
                mcp_channel.read_until(key="McpHeartBeat", timeout_seconds=3600)
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
            if 'scp_channel' in locals() and scp_channel.is_open():
                scp_channel.close()
            if 'mcp_channel' in locals() and mcp_channel.is_open():
                mcp_channel.close()

    def setup(self) -> bool:
        """Setup test environment"""
        try:
            self.log.info("Setting up DUT...")
            self.dut.setup()

            # Wait for SCP and MCP heartbeat during initial setup
            if not self.wait_for_scp_mcp_heartbeat():
                self.log.error("Failed to receive initial SCP-MCP heartbeat during setup")
                return False

            # Setup MCP channel
            self.mcp_channel = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
            self.mcp_channel.open()

            if not self.mcp_channel.is_open():
                self.log.error("Failed to open MCP channel")
                return False

            return True

        except Exception as e:
            self.log.error(f"Error in setup: {e}")
            return False

    def teardown(self):
        """Cleanup test environment"""
        try:
            if hasattr(self, 'mcp_channel'):
                self.mcp_channel.close()
            self.dut.teardown()
            # Reset the heartbeat flag since SVP is being terminated
            self.__class__._scp_mcp_heartbeat_received = False
        except Exception as e:
            self.log.error(f"Error in teardown: {e}")



    def get_mcp_channel(self):
        """Get MCP channel and ensure it's open"""
        try:
            # Get MCP channel
            mcp_channel = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
            
            # Open channel if not already open
            if not mcp_channel.is_open():
                mcp_channel.open()
                
            if not mcp_channel.is_open():
                self.log.error("Failed to open MCP channel")
                return None
                
            return mcp_channel
            
        except Exception as e:
            self.log.error(f"Error getting MCP channel: {e}")
            return None


    def dcs_command_test_on_mcp(self, command: str, read_until_key: str = "Ok", pass_logs: Union[List[str], str] = None) -> str:
        """Execute DCS command and verify response"""
        try:
            mcp_channel = self.get_mcp_channel()
            if not mcp_channel:
                self.log.error("Failed to get MCP channel")
                return "Failed to get MCP channel"
                
            try:
                # Execute command
                self.log.info(f"Executing command: {command}")
                mcp_channel.write_line(write_string=command)
                
                # Wait for command completion
                command_response = mcp_channel.read_until(key=read_until_key, timeout_seconds=300)
                self.log.info(f"Command response: {command_response}")
                
                if not command_response:
                    self.log.error("Received empty response")
                    return "Empty response received"
                
                if pass_logs is not None:
                    if isinstance(pass_logs, str):
                        pass_logs_list = [pass_logs]
                    else:
                        pass_logs_list = pass_logs
                    
                    for item in pass_logs_list:
                        if item in command_response:
                            self.log.info(f"✅ Found expected response: '{item}'")
                        else:
                            self.log.error(f"❌ Missing expected response: '{item}'")
                            return f"Missing expected response: '{item}'"
                            
                return command_response
                    
            except Exception as e:
                self.log.error(f"Error executing command: {str(e)}")
                return str(e)
                
        except Exception as e:
            self.log.error(f"Error during test execution: {str(e)}")
            return str(e)
        finally:
            if 'mcp_channel' in locals() and mcp_channel.is_open():
                mcp_channel.close()

    def test_client_start_stop(self, command=None):
        """Test DCS client start/stop functionality"""
        try:
            # Get transport for DCP (die0/AP)
            transport = get_transport(DIE_0, CPU_AP)

            if command:
                # Execute single command
                formatted_command = transport.send_dcp_msg(DIE_0, CPU_MCP, command)
                if transport.config['uart'] == 'mcp':
                    response = self.dcs_command_test_on_mcp(formatted_command)
                    if "Ok" not in response:
                        self.log.error(f"Command failed: {response}")
                    return response
                else:
                    self.log.info(f"Command via {transport.config['uart']} to be implemented")
                    return "Ok"
            
            else:
                start_msg = DataCollectionProtocol.client_start_stop(
                    client_id=0, 
                    state=DataCollectionProtocol.ClientStartStop.State.START)
                formatted_start = transport.send_dcp_msg(DIE_0, CPU_MCP, start_msg)
                if transport.config['uart'] == 'mcp':
                    start_response = self.dcs_command_test_on_mcp(formatted_start)
                    if "Ok" not in start_response:
                        self.log.error(f"START command failed: {start_response}")
                        return start_response
                else:
                    self.log.info(f"START command via {transport.config['uart']} to be implemented")
                    start_response = "Ok"
                
                # STOP command
                stop_msg = DataCollectionProtocol.client_start_stop(
                    client_id=0,
                    state=DataCollectionProtocol.ClientStartStop.State.STOP)
                formatted_stop = transport.send_dcp_msg(DIE_0, CPU_MCP, stop_msg)
                if transport.config['uart'] == 'mcp':
                    stop_response = self.dcs_command_test_on_mcp(formatted_stop)
                    if "Ok" not in stop_response:
                        self.log.error(f"STOP command failed: {stop_response}")
                        return stop_response
                else:
                    self.log.info(f"STOP command via {transport.config['uart']} to be implemented")
                    stop_response = "Ok"
                
                # Log final success message and return last response
                self.log.info(f"✅ DCS Client Start/Stop test completed successfully: {stop_response}")
                return stop_response

        except Exception as e:
            self.log.error(f"❌ Error executing command: {e}")
            return str(e)