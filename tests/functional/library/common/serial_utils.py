# Copyright (c) Microsoft Corporation. All rights reserved.
#
# serial_utils.py - Utility class for managing serial communication with SCP and MCP channels.
# Provides methods for sending commands, reading responses, and waiting for heartbeat signals.
from typing import Union, List, Optional
import time
import sys
from pythia.tdk.echofalls.constants.dut_types import DeviceType
import logging
from fpfw_automation_primitives.serial.telnet import (
    Telnet_,
)

logger = logging.getLogger(__name__)


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
        if self.log:
            self.log.info("MCP channel opened successfully")

    def open_scp_channel(self):
        """
        Opens the SCP communication channel.
        """
        if not self.scp_channel.is_open():
            self.scp_channel.open()
            if not self.scp_channel.is_open():
                raise RuntimeError("Failed to open SCP communication channel")
        if self.log:
            self.log.info("SCP channel opened successfully")

    def run_command_on_mcp(
        self,
        command: str,
        read_until_key: str,
        pass_logs: Optional[Union[List[str], str]] = None,
    ) -> Optional[str]:
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
        if self.log:
            self.log.info(f"Running command on MCP with command: {command}")
        try:
            self.mcp_channel.open()
            if not self.mcp_channel.is_open():
                if self.log:
                    self.log.error("Failed to open MCP communication channel")
                return None

            if self.log:
                self.log.info(f"Executing command: {command}")
            self.mcp_channel.write_line(write_string=command)

            command_response = self.read_mcp_serial_until(read_until_key)
            if command_response is None:
                return None

            if pass_logs is not None:
                pass_logs_list = (
                    [pass_logs] if isinstance(pass_logs, str) else pass_logs
                )
                for item in pass_logs_list:
                    if item in command_response:
                        if self.log:
                            self.log.info(f"Found: '{item}'")
                    else:
                        if self.log:
                            self.log.error(f"Not Found: '{item}'")
                        return None

            return command_response

        except Exception as e:
            if self.log:
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
        if self.log:
            self.log.info(f"Running command: {command}")
        try:
            if self.dut:
                dut_type = self.dut.get_dut_type()
            else:
                raise ValueError("DUT instance is None")
            if dut_type in [DeviceType.BIGFPGA, DeviceType.SVP, DeviceType.RVP]:
                if self.log:
                    self.log.info(f"Testing and opening channel for {dut_type}")
                self.scp_channel.open()
                if not self.scp_channel.is_open():
                    if self.log:
                        self.log.error("Failed to open core communication channel")
                    return None

                if self.log:
                    self.log.info(f"Executing command: {command}")
                self.scp_channel.write_line(write_string=command)

                command_response = self.read_scp_serial_until(read_until_key)
                if command_response is None:
                    return None

                return command_response
            else:
                raise ValueError(f"Unsupported DUT type: {dut_type}")

        except Exception as e:
            if self.log:
                self.log.error(f"Error during command execution: {str(e)}")
            return None
        finally:
            self.scp_channel.close()

    def read_scp_serial_until(
        self, read_until_key: str, timeout_seconds: int = 60
    ) -> Optional[str]:
        """
        Read from the SCP serial channel until a specific key is found or timeout occurs.

        Args:
            read_until_key (str): Key to wait for in the response.
            timeout_seconds (int): Timeout in seconds.

        Returns:
            Optional[str]: The response if successful; None otherwise.
        """
        return self._read_serial_until(
            self.scp_channel, read_until_key, timeout_seconds
        )

    def read_mcp_serial_until(
        self, read_until_key: str, timeout_seconds: int = 60
    ) -> Optional[str]:
        """
        Read from the MCP serial channel until a specific key is found or timeout occurs.

        Args:
            read_until_key (str): Key to wait for in the response.
            timeout_seconds (int): Timeout in seconds.

        Returns:
            Optional[str]: The response if successful; None otherwise.
        """
        return self._read_serial_until(
            self.mcp_channel, read_until_key, timeout_seconds
        )

    def _read_serial_until(
        self, channel, read_until_key: str, timeout_seconds: int
    ) -> Optional[str]:
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
                if self.log:
                    self.log.error("Failed to open communication channel")
                return None
            command_response = channel.read_until(
                key=read_until_key, timeout_seconds=timeout_seconds
            )
            if self.log:
                self.log.debug("Received Response Successfully from UART . . .")
                self.log.debug(command_response)
            return command_response
        except Exception as e:
            error_msg = f"Error reading UART: {e}"
            buffer_content = (
                e.incomplete_line if hasattr(e, "incomplete_line") else str(e)
            )

            if self.log:
                self.log.error(
                    f"{error_msg}\n"
                    f"=== REMAINING BUFFER ===\n"
                    f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}\n"
                    f"Read until key: {read_until_key}\n"
                    f"Buffer content:\n{buffer_content}\n"
                    f"=== END BUFFER ==="
                )
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

            if self.log:
                self.log.info("Waiting for initial SCP Heartbeat Message...")
            try:
                self.scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=1800)
                if self.log:
                    self.log.info("SCP Heartbeat received successfully")
            except Exception as e:
                if self.log:
                    self.log.error(f"Failed to receive SCP heartbeat: {str(e)}")
                return False

            if self.log:
                self.log.info("Waiting for initial MCP Heartbeat Message...")
            try:
                self.mcp_channel.read_until(key="McpHeartBeat", timeout_seconds=1800)
                if self.log:
                    self.log.info("MCP Heartbeat received successfully")
            except Exception as e:
                if self.log:
                    self.log.error(f"Failed to receive MCP heartbeat: {str(e)}")
                return False

            self._scp_mcp_heartbeat_received = True
            return True
        except Exception as e:
            if self.log:
                self.log.error(f"Unexpected error in heartbeat check: {str(e)}")
            return False


class SerialChannelBase:
    """
    A utility class to manage serial communication with a uart channel.
    Provides methods to send commands, read responses, and wait for heartbeat signals.
    Uses segregated interfaces to follow Interface Segregation Principle.
    """

    def __init__(self, channel, name: str, ready_text: str):
        """
        Initialize the SerialChannelBase with communication channel and required parameters.

        Args:
            channel: Serial communication channel.
            name: Name for the channel.
            ready_text: Ready text to wait for during initialization.
        """
        self.channel = channel
        self.name: str = name
        self._ready_text_received: bool = False
        self.wait_for_ready_text: str = ready_text

    @property
    def is_ready_text_received(self) -> bool:
        """Check if the ready text has been received."""
        return self._ready_text_received

    def open_channel(self) -> None:
        """
        Opens the communication channel.
        """
        if not self.channel.is_open():
            self.channel.open()
            if not self.channel.is_open():
                raise RuntimeError(f"Failed to open {self.name} communication channel")
        logger.info(f"{self.name} channel opened successfully")

    def close_channel(self) -> None:
        """
        Close the communication channel.
        """
        if self.channel and self.channel.is_open():
            self.channel.close()
            logger.info(f"{self.name} channel closed")

    def run_command(
        self, command: str, read_until_key: Optional[str] = None
    ) -> Optional[str]:
        """
        Execute a command on the channel and optionally read the response until a key is found.

        Args:
            command (str): Command to send to channel.
            read_until_key (Optional[str]): Key to wait for in the response. If None, skips reading response.

        Returns:
            Optional[str]: The command response if successful and read_until_key provided; None otherwise.
        """
        logger.debug(f"Running command: {command}")
        try:
            logger.debug(f"Executing command: {command}")
            self.channel.write_line(write_string=command)

            if read_until_key is not None:
                command_response = self._read_serial_until(
                    self.channel, read_until_key, 60
                )
                if command_response is None:
                    return None
                return command_response
            else:
                logger.debug("Skipping response reading as read_until_key is None")
                return None

        except Exception as e:
            logger.error(f"Error during command execution: {str(e)}")
            # Close channel on error to prevent corrupted state
            if self.channel and self.channel.is_open():
                self.channel.close()
                logger.debug("Channel closed due to error")
            raise
        # Note: Channel intentionally left open for subsequent commands on success

    def read_channel_until(
        self, read_until_key: str, timeout_seconds: int = 60
    ) -> Optional[str]:
        """
        Read from the channel until a specific key is found or timeout occurs.

        Args:
            read_until_key (str): Key to wait for in the response.
            timeout_seconds (int): Timeout in seconds.

        Returns:
            Optional[str]: The response if successful; None otherwise.
        """
        return self._read_serial_until(self.channel, read_until_key, timeout_seconds)

    def _read_serial_until(
        self, channel, read_until_key: str, timeout_seconds: int
    ) -> Optional[str]:
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
            command_response = channel.read_until(
                key=read_until_key, timeout_seconds=timeout_seconds
            )
            logger.debug("Received Response Successfully from UART . . .")
            logger.debug(command_response)
            return command_response

        except Exception as e:
            error_msg = f"Error reading UART {self.name}: {e}"
            buffer_content = (
                e.incomplete_line if hasattr(e, "incomplete_line") else str(e)
            )

            logger.error(
                f"{error_msg}\n"
                f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}\n"
                f"Read until key: {read_until_key}\n"
                f"=== UART DUMP  ===\n"
                f"Buffer content:\n{buffer_content}\n"
                f"=== END UART DUMP ==="
            )
            return None

    def wait_for_ready_text_message(self) -> bool:
        """
        Wait for the ready text message indicating the channel is ready.

        Returns:
            bool: True if received, False otherwise.
        """
        try:
            logger.info(f"Waiting for initial ready text {self.wait_for_ready_text}...")
            try:
                self.channel.read_until(
                    key=self.wait_for_ready_text, timeout_seconds=1800
                )
                logger.info("Ready text received successfully")
            except Exception as e:
                logger.error(f"Failed to receive ready text: {str(e)}")
                return False

            self._ready_text_received = True
            return True
        except Exception as e:
            logger.error(f"Unexpected error in ready text check: {str(e)}")
            return False


# NOTE: the following code in main() is used for manual testing.  Developers may construct sequences as needed.
# commented out blocks are left for reference
# running with the channel open in either SVP or Putty will not work.  there will be data corruption due to
# multiple  connections
# launch SVP with the GUI and close the uart window. 'runsvpgui -SimConfig chie_bins_single_die_dat' or dual die
# The UART window may re-opened for manual testing, but will need to be closed again for this script to work
# For example, for SCP_UART, In the SVP Design Hierarchy navigate to /KingsgateSVP/DIE_0/RMSS/SCP/SCP_UART,
# right click, and select 'Connect Terminal to Uart'
#
# to run the main functionality below from a repo environment
# & (Join-Path ([System.Environment]::GetEnvironmentVariable("REPO_APP_PATH_python.win64", "Process")) "/tools/python.exe") tests/functional/library/common/serial_utils.py # noqa: E501
#
def main():
    print("This is the main entry point of serial_utils.py")

    # for manual testing redirect loggers to stdout
    logging.basicConfig(
        level=logging.DEBUG,  # Set the logging level (DEBUG, INFO, etc.)
        handlers=[logging.StreamHandler(sys.stdout)],  # Redirect to stdout
    )

    # UEFI is on AP_NS_UART0
    # /KingsgateSVP/DIE_0/CSS/HndBaseElement/AP_NS_UART0
    telnet_port = Telnet_(host="localhost", port="4258", encoding="UTF-8")
    telnet_port.open()

    uefi_channel = SerialChannelBase(
        telnet_port, "UEFI_Channel", "UEFI Interactive Shell"
    )
    uefi_channel.open_channel()
    # uefi_channel.wait_for_ready_text_message()
    uefi_channel.run_command("help", "Shell>")


if __name__ == "__main__":
    main()
