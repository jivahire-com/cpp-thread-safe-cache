# Copyright (c) Microsoft Corporation. All rights reserved.
#
# uefi_shell.py - UEFI Shell utility class for managing serial communication with UEFI channel.
# Provides methods for sending commands, reading responses, and waiting for ready signals.

import logging
import sys
from typing import Optional
from fpfw_automation_primitives.serial.telnet import (
    Telnet_,
)  # noqa: F401
from fpfw_automation_primitives.serial.direct import (
    DirectSerial,
)  # noqa: F401
from serial_utils import SerialChannelBase

logger = logging.getLogger(__name__)


class UefiShell(SerialChannelBase):
    """
    A utility class to manage serial communication with UEFI channel.
    Inherits from SerialChannelBase and provides UEFI-specific functionality.
    """

    def __init__(
        self,
        channel,
        name: str = "UEFI_Shell",
        ready_text: str = "UEFI Interactive Shell",
    ):
        """
        Initialize the UefiShell with communication channel and UEFI-specific parameters.

        Args:
            channel: Serial communication channel.
            name: Name for the channel (defaults to "UEFI_Shell").
            ready_text: Ready text to wait for during initialization (defaults to "UEFI Interactive Shell").
        """
        super().__init__(channel, name, ready_text)

    def run_uefi_command(
        self, command: str, read_until_key: str = "Shell>"
    ) -> Optional[str]:
        """
        Execute a UEFI command and read the response until Shell prompt.

        Args:
            command (str): UEFI command to execute.
            read_until_key (str): Key to wait for in the response (defaults to "Shell>").

        Returns:
            Optional[str]: The command response if successful; None otherwise.
        """
        return self.run_command(command, read_until_key)

    def wait_for_shell_ready(self) -> bool:
        """
        Wait for UEFI shell to be ready.

        Returns:
            bool: True if shell is ready, False otherwise.
        """
        return self.wait_for_ready_text_message()

    def turn_on_core(self, cores: str) -> Optional[str]:
        """
        Turn on CPU cores with flexible string-based input formats.

        Args:
            cores: Core specification as string - supports:
                - Single core: "5"
                - Range: "5-8"
                - All cores: "ALL"

        Returns:
            Optional[str]: Command response if successful; None otherwise

        Examples:
            turn_on_core("5")        # Single core
            turn_on_core("5-8")      # Range of cores
            turn_on_core("ALL")      # All cores
        """
        cores_arg = self._parse_core_specification(cores)
        command = f"FS0:PsciTest.efi CORE_ON  {cores_arg}"
        response = self.run_uefi_command(command, "remove-symbol-file")

        if response:
            # Validate that expected cores were enabled
            expected_cores = self._get_expected_core_list(cores)
            if cores.strip().upper() == "ALL":
                logger.info(
                    "ALL cores command executed - skipping individual core validation"
                )
            elif self._validate_core_response(
                response, expected_cores, "Hello from CPU {core}"
            ):
                logger.info("All expected cores responded successfully")
            else:
                logger.error(f"Not all expected cores responded -  {response}")

        return response

    def turn_off_core(self, cores: str) -> Optional[str]:
        """
        Turn off CPU cores with flexible string-based input formats.

        Args:
            cores: Core specification as string - supports:
                - Single core: "5"
                - Range: "5-8"
                - All cores: "ALL"

        Returns:
            Optional[str]: Command response if successful; None otherwise

        Examples:
            turn_off_core("5")        # Single core
            turn_off_core("5-8")      # Range of cores
            turn_off_core("ALL")      # All cores
        """
        cores_arg = self._parse_core_specification(cores)
        command = f"FS0:PsciTest.efi CORE_OFF  {cores_arg}"
        response = self.run_uefi_command(command, "remove-symbol-file")

        if response:
            # Validate that expected cores were turned off
            expected_cores = self._get_expected_core_list(cores)
            if cores.strip().upper() == "ALL":
                logger.info(
                    "ALL cores command executed - skipping individual core validation"
                )
            elif self._validate_core_response(
                response, expected_cores, "CPU {core} says bye"
            ):
                logger.info("All expected cores responded successfully")
            else:
                logger.error(f"Not all expected cores responded -  {response}")

        return response

    def suspend_core(self, cores: str, suspend_state: str) -> Optional[str]:
        """
        Suspend CPU cores with specified suspend state.

        Args:
            cores: Core specification as string - supports:
                - Single core: "5"
                - Range: "5-8"
                Note: ALL option is not supported for suspend operations
            suspend_state: Suspend state - must be one of: C1, C2, C3, C4

        Returns:
            Optional[str]: Command response if successful; None otherwise

        Examples:
            suspend_core("5", "C1")      # Single core to C1 state
            suspend_core("5-8", "C2")    # Range of cores to C2 state
        """
        # Validate suspend state
        valid_states = ["C1", "C2", "C3", "C4"]
        suspend_state_clean = suspend_state.strip().upper()
        if suspend_state_clean not in valid_states:
            raise ValueError(
                f"Invalid suspend state: {suspend_state}. Must be one of: {valid_states}"
            )

        # Parse cores but don't allow ALL
        cores_clean = cores.strip().upper()
        if cores_clean == "ALL":
            raise ValueError("ALL option is not supported for suspend_core operations")

        cores_arg = self._parse_core_specification(cores)
        command = f"FS0:PsciTest.efi CORE_SUSPEND  {cores_arg} {suspend_state_clean}"
        response = self.run_uefi_command(command, "remove-symbol-file")

        if response:
            # Validate that expected cores were suspended
            expected_cores = self._get_expected_core_list(cores)
            if self._validate_core_response(
                response, expected_cores, "Siesta time - CPU {core}"
            ):
                logger.info("All expected cores responded successfully")
            else:
                logger.error(f"Not all expected cores responded -  {response}")

        return response

    def resume_core(self, cores: str) -> Optional[str]:
        """
        Resume CPU cores from suspended state.

        Args:
            cores: Core specification as string - supports:
                - Single core: "5"
                - Range: "5-8"
                Note: ALL option is not supported for resume operations

        Returns:
            Optional[str]: Command response if successful; None otherwise

        Examples:
            resume_core("5")      # Resume single core
            resume_core("5-8")    # Resume range of cores
        """
        # Parse cores but don't allow ALL
        cores_clean = cores.strip().upper()
        if cores_clean == "ALL":
            raise ValueError("ALL option is not supported for resume_core operations")

        cores_arg = self._parse_core_specification(cores)
        command = f"FS0:PsciTest.efi CORE_RESUME  {cores_arg}"
        response = self.run_uefi_command(command, "remove-symbol-file")

        if response:
            # Validate that expected cores were resumed
            expected_cores = self._get_expected_core_list(cores)
            if self._validate_core_response(
                response, expected_cores, "Hello from CPU {core}"
            ):
                logger.info("All expected cores responded successfully")
            else:
                logger.error(f"Not all expected cores responded -  {response}")

        return response

    def _parse_core_specification(self, cores: str) -> str:
        """
        Validate and normalize core specification string.

        Args:
            cores: Core specification string

        Returns:
            str: Validated core specification for shell command

        Raises:
            ValueError: If core specification format is invalid
        """
        if not isinstance(cores, str):
            raise ValueError(f"Core specification must be a string, got {type(cores)}")

        cores_clean = cores.strip().upper()

        if cores_clean == "ALL":
            return "ALL"
        elif "-" in cores_clean:
            # Validate range format (e.g., "5-8")
            parts = cores_clean.split("-")
            if len(parts) != 2:
                raise ValueError(
                    f"Invalid range format: {cores}. Expected format: '5-8'"
                )
            try:
                start, end = int(parts[0]), int(parts[1])
                if start >= end:
                    raise ValueError(
                        f"Invalid range: start ({start}) must be less than end ({end})"
                    )
                return f"{start} - {end}"
            except ValueError as e:
                if "invalid literal" in str(e):
                    raise ValueError(
                        f"Invalid range format: {cores}. Both start and end must be numbers"
                    )
                raise
        else:
            # Single core
            try:
                core_num = int(cores_clean)
                return str(core_num)
            except ValueError:
                raise ValueError(
                    f"Invalid core specification: {cores}. "
                    f"Must be a number, range, or 'ALL'"
                )

    def _get_expected_core_list(self, cores: str) -> list:
        """
        Get the list of cores that should be enabled based on input specification.

        Args:
            cores: Original core specification string

        Returns:
            list: List of expected core numbers
        """
        cores_clean = cores.strip().upper()

        if cores_clean == "ALL":
            # For "ALL", we can't predict exact cores without system info
            # Return empty list to skip validation for ALL case
            return []
        elif "-" in cores_clean:
            # Range format
            parts = cores_clean.split("-")
            start, end = int(parts[0]), int(parts[1])
            return list(range(start, end + 1))
        else:
            # Single core
            return [int(cores_clean)]

    def _validate_core_response(
        self, response: str, expected_cores: list, pattern: str
    ) -> bool:
        """
        Validate that the response contains specified pattern messages for expected cores.

        Args:
            response: Command response text
            expected_cores: List of expected core numbers
            pattern: Pattern string with {core} placeholder (e.g., "Hello from CPU {core}")

        Returns:
            bool: True if all expected cores are found in response
        """
        if not expected_cores:
            # Skip validation for ALL case - just check if any pattern messages exist
            import re

            # Convert pattern to regex by replacing {core} with \d+
            regex_pattern = pattern.replace("{core}", r"\d+")
            matches = re.findall(regex_pattern, response)
            if matches:
                logger.info(f"Found {len(matches)} core response messages: {matches}")
                return True
            else:
                logger.warning(
                    f"No '{pattern}' pattern messages found in response for ALL cores"
                )
                return False

        # Check for specific cores
        found_cores = []
        missing_cores = []

        for core_num in expected_cores:
            expected_message = pattern.format(core=core_num)
            if expected_message in response:
                found_cores.append(core_num)
                logger.debug(f"Received: {expected_message}")
            else:
                missing_cores.append(core_num)
                logger.debug(f"Not found: {expected_message}")

        return len(missing_cores) == 0


# NOTE: the following code in main() is used for manual testing.  Developers may construct sequences as needed.
# commented out blocks are left for reference
# running with the channel open in either SVP or Putty will not work.  there will be data corruption due to
# multiple  connections
# launch SVP with the GUI and close the uart window. 'runsvpgui -SimConfig chie_bins_single_die_dat' or dual die
# The UART window may re-opened for manual testing, but will need to be closed again for this script to work
# For example, for UEFI (AP_NS_UART0), In the SVP Design Hierarchy navigate to
# /KingsgateSVP/DIE_0/CSS/HndBaseElement/AP_NS_UART0, right click, and select 'Connect Terminal to Uart'
#
# to run the main functionality below from a repo environment
# & (Join-Path ([System.Environment]::GetEnvironmentVariable("REPO_APP_PATH_python.win64", "Process")) "/tools/python.exe") tests/functional/library/common/uefi_shell.py # noqa: E501
#
def main():
    print("This is the main entry point of uefi_shell.py")

    # for manual testing redirect loggers to stdout
    logging.basicConfig(
        level=logging.DEBUG,  # Set the logging level (DEBUG, INFO, etc.)
        handlers=[logging.StreamHandler(sys.stdout)],  # Redirect to stdout
    )

    # UEFI is on AP_NS_UART0
    # SVP Config
    # /KingsgateSVP/DIE_0/CSS/HndBaseElement/AP_NS_UART0
    # telnet_port = Telnet_(host="localhost", port="4258", encoding="UTF-8")
    # telnet_port.open()
    # uefi_shell = UefiShell(telnet_port)

    # Direct config on silicon
    direct_port = DirectSerial(id="COM8", baud_rate="115200", throttle_seconds=0.005)
    direct_port.open()
    uefi_shell = UefiShell(direct_port)

    uefi_shell.open_channel()
    # Wait for UEFI shell to be ready (optional, uncomment if needed)
    # uefi_shell.wait_for_shell_ready()
    # uefi_shell.run_uefi_command("help")

    uefi_shell.turn_off_core("ALL")
    # uefi_shell.turn_on_core("2 - 5")

    # uefi_shell.turn_off_core("2")
    # uefi_shell.turn_off_core("3")
    # uefi_shell.turn_off_core("4")
    # uefi_shell.turn_off_core("5")

    uefi_shell.turn_on_core("6 - 9")
    uefi_shell.suspend_core("6 - 9", "C2")
    uefi_shell.resume_core("6 - 9")
    uefi_shell.turn_off_core("6 - 9")

    # Clean up - close the channel when done
    uefi_shell.close_channel()


if __name__ == "__main__":
    main()
