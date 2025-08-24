# Copyright (c) Microsoft Corporation. All rights reserved.
#
# serial_utils.py - Utility class for managing serial communication with SCP and MCP channels.
# Provides methods for sending commands, reading responses, and waiting for heartbeat signals.
from typing import Union, List, Optional
import time
from pythia.tdk.echofalls.constants.dut_types import DeviceType

class SerialUtility:
    """
    A utility class to manage serial communication with SCP and MCP channels.
    Provides methods to send commands, read responses, and wait for heartbeat signals.
    """

    def __init__(self, scp_channel, mcp_channel, logger=None, dut=None):
        """
        Initialize the SerialUtility with communication channels and optional logger and DUT.

        Args:
            scp_channel: Serial communication channel for SCP.
            mcp_channel: Serial communication channel for MCP.
            logger: Optional logger instance for logging messages.
            dut: Optional DUT object to determine device type.
        """
        self.scp_channel = scp_channel
        self.mcp_channel = mcp_channel
        self.log = logger
        self.dut = dut
        self._scp_mcp_heartbeat_received = False
    
    def open_mcp_channel(self):
        """
        Opens the MCP communication channel.
        """
        if not self.mcp_channel.is_open():
            self.mcp_channel.open()
            if not self.mcp_channel.is_open():
                raise RuntimeError("Failed to open MCP communication channel")
        self.log.info("MCP channel opened successfully")
    
    def open_scp_channel(self):
        """
        Opens the SCP communication channel.
        """
        if not self.scp_channel.is_open():
            self.scp_channel.open()
            if not self.scp_channel.is_open():
                raise RuntimeError("Failed to open SCP communication channel")
        self.log.info("SCP channel opened successfully")

    def run_command_on_mcp(self, command: str, read_until_key: str, pass_logs: Union[List[str], str] = None) -> Optional[str]:
        """
        Execute a command on the MCP channel and read the response until a key is found.
        Optionally check for expected log entries in the response.

        Args:
            command (str): Command to send to MCP.
            read_until_key (str): Key to wait for in the response.
            pass_logs (Union[List[str], str], optional): Expected log entries to validate.

        Returns:
            Optional[str]: The command response if successful; None otherwise.
        """
        self.log.info(f"Running command on MCP with command: {command}")
        try:
            self.mcp_channel.open()
            if not self.mcp_channel.is_open():
                self.log.error("Failed to open MCP communication channel")
                return None

            self.log.info(f"Executing command: {command}")
            self.mcp_channel.write_line(write_string=command)

            command_response = self.read_mcp_serial_until(read_until_key)
            if command_response is None:
                return None

            if pass_logs is not None:
                pass_logs_list = [pass_logs] if isinstance(pass_logs, str) else pass_logs
                for item in pass_logs_list:
                    if item in command_response:
                        self.log.info(f"Found: '{item}'")
                    else:
                        self.log.error(f"Not Found: '{item}'")
                        return None

            return command_response

        except Exception as e:
            self.log.error(f"Error during MCP command execution: {str(e)}")
            return None
        finally:
            self.mcp_channel.close()

    def run_command_on_scp(self, command: str, read_until_key: str) -> Optional[str]:
        """
        Execute a command on the SCP channel and read the response until a key is found.

        Args:
            command (str): Command to send to SCP.
            read_until_key (str): Key to wait for in the response.

        Returns:
            Optional[str]: The command response if successful; None otherwise.
        """
        self.log.info(f"Running command: {command}")
        try:
            dut_type = self.dut.get_dut_type()
            if dut_type in [DeviceType.BIGFPGA, DeviceType.SVP]:
                self.log.info(f"Testing and opening channel for {dut_type}")
                self.scp_channel.open()
                if not self.scp_channel.is_open():
                    self.log.error("Failed to open core communication channel")
                    return None

                self.log.info(f"Executing command: {command}")
                self.scp_channel.write_line(write_string=command)

                command_response = self.read_scp_serial_until(read_until_key)
                if command_response is None:
                    return None

                return command_response
            else:
                raise ValueError(f"Unsupported DUT type: {dut_type}")

        except Exception as e:
            self.log.error(f"Error during command execution: {str(e)}")
            return None
        finally:
            self.scp_channel.close()

    def read_scp_serial_until(self, read_until_key: str, timeout_seconds: int = 60) -> Optional[str]:
        """
        Read from the SCP serial channel until a specific key is found or timeout occurs.

        Args:
            read_until_key (str): Key to wait for in the response.
            timeout_seconds (int): Timeout in seconds.

        Returns:
            Optional[str]: The response if successful; None otherwise.
        """
        return self._read_serial_until(self.scp_channel, read_until_key, timeout_seconds)

    def read_mcp_serial_until(self, read_until_key: str, timeout_seconds: int = 60) -> Optional[str]:
        """
        Read from the MCP serial channel until a specific key is found or timeout occurs.

        Args:
            read_until_key (str): Key to wait for in the response.
            timeout_seconds (int): Timeout in seconds.

        Returns:
            Optional[str]: The response if successful; None otherwise.
        """
        return self._read_serial_until(self.mcp_channel, read_until_key, timeout_seconds)

    def _read_serial_until(self, channel, read_until_key: str, timeout_seconds: int) -> Optional[str]:
        """
        Private method to handle the actual serial reading logic.

        Args:
            channel: Serial channel to read from.
            read_until_key (str): Key to wait for in the response.
            timeout_seconds (int): Timeout in seconds.

        Returns:
            Optional[str]: The response if successful; None otherwise.
        """
        try:
            channel.open()
            if not channel.is_open():
                self.log.error("Failed to open communication channel")
                return None
            command_response = channel.read_until(key=read_until_key, timeout_seconds=timeout_seconds)
            self.log.debug("Received Response Successfully from UART . . .")
            self.log.debug(command_response)
            return command_response
        except Exception as e:
            error_msg = f"Error reading UART: {e}"
            buffer_content = e.incomplete_line if hasattr(e, 'incomplete_line') else str(e)

            self.log.error(f"{error_msg}\n"
                        f"=== REMAINING BUFFER ===\n"
                        f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}\n"
                        f"Read until key: {read_until_key}\n"
                        f"Buffer content:\n{buffer_content}\n"
                        f"=== END BUFFER ===")
            return None

    def wait_for_scp_mcp_heartbeat(self) -> bool:
        """
        Wait for both SCP and MCP heartbeat messages.

        Returns:
            bool: True if both heartbeats are received successfully, False otherwise.
        """
        try:
            self.scp_channel.open()
            self.mcp_channel.open()

            self.log.info("Waiting for initial SCP Heartbeat Message...")
            try:
                self.scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=1800)
                self.log.info("SCP Heartbeat received successfully")
            except Exception as e:
                self.log.error(f"Failed to receive SCP heartbeat: {str(e)}")
                return False

            self.log.info("Waiting for initial MCP Heartbeat Message...")
            try:
                self.mcp_channel.read_until(key="McpHeartBeat", timeout_seconds=1800)
                self.log.info("MCP Heartbeat received successfully")
            except Exception as e:
                self.log.error(f"Failed to receive MCP heartbeat: {str(e)}")
                return False

            self._scp_mcp_heartbeat_received = True
            return True
        except Exception as e:
            self.log.error(f"Unexpected error in heartbeat check: {str(e)}")
            return False
