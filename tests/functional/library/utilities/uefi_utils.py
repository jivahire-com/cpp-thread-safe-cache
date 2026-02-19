"""
UEFI utility helpers.
Provides reusable functions for executing UEFI CLI commands.
"""

import time
from typing import Any


class UEFI_helper:
    def __init__(self, dut, log):
        assert dut is not None
        assert log is not None

        self.dut = dut
        self.log = log
        self.output = ""

    def reset_and_boot_uefi(self, apns_cli: Any, rscm_helper: Any) -> bool:
        """
        Reset SOC by BMC and APNS boot to UEFI.
        :param apns_cli: Any
        :param rscm_helper: Any : from RscmHelperLibrary import RscmHelperLibrary
        :return:  bool, True if shell is up and interactive, False otherwise.

        Raises:
            TimeoutError: If the UEFI shell is not up within the timeout.
        """

        assert apns_cli is not None
        assert rscm_helper is not None

        self.log.info(f"Setup boot options")
        rscm_helper.rscm_set_boot_option(option="ConfApp")

        self.log.info(f"RSCM_helper do a SOC reset...")
        rscm_helper.rscm_soc_reset()

        self.log.info(f"Booting UEFI Shell...")
        is_uefi_shell_up = self.wait_for_uefi_shell_up(apns_cli=apns_cli)

        return is_uefi_shell_up

    def wait_for_uefi_shell_up(self, apns_cli: Any, timeout: int = 1200) -> bool:
        """
        Wait for UEFI shell to be up and interactive.
        :param apns_cli: Any
        :param timeout: int: Timeout in seconds. UEFI boot is expected to complete within 15 mins.
        :return:  bool, True if shell is up and interactive, False otherwise.

        Raises:
            TimeoutError: If the UEFI shell is not up within the timeout.
        """

        assert apns_cli is not None

        start_time = time.time()
        while time.time() - start_time < timeout:
            if self.is_uefi_shell_up(apns_cli):
                self.output = self.write_console_interaction_command_read_until(
                    apns_cli=apns_cli, command="\r\n", expected_key="Shell>", timeout=45
                )

                if "Shell>" not in self.output:
                    self.log.error(
                        f"Execute UEFI shell command failed : CRLF : output = {self.output}"
                    )
                    break

                CMD = "ver"
                CMD_ARGS = []
                self.execute_uefi_command(
                    apns_cli=apns_cli, command=CMD, command_args=CMD_ARGS
                )
                self.output = apns_cli.read_until(key="UEFI", timeout_seconds=45)

                if "UEFI" in self.output:
                    self.log.info(f"UEFI shell is up")
                    return True
                else:
                    self.log.error(
                        f"Execute UEFI shell command failed : ver : output = {self.output}"
                    )
                    break

        try:
            self.output = self.write_console_interaction_command_read_until(
                apns_cli=apns_cli, command="\r\n", expected_key="SAC>", timeout=5
            )
            self.log.error(
                f"UEFI boot failed. Entering to SAC>. output = {self.output}"
            )
        except TimeoutError:
            self.log.error(f"Timeout for waiting SAC>")

        self.log.error(f"Timeout waiting for UEFI shell to be up")
        raise TimeoutError("Timeout waiting for UEFI shell to be up")

    def is_uefi_shell_up(self, apns_cli: Any) -> bool:
        """
        Checks if UEFI shell is up and interactive.
        :param          apns_cli:
        :return:        bool, True if shell is up and interactive, False otherwise.
        """

        assert apns_cli is not None

        # Step
        self.log.info(f"Entering UEFI shell...")
        try:
            self.log.info(f"Waiting for boot menu")
            self.output = apns_cli.read_until(
                key="Available Options", timeout_seconds=300
            )
            self.output = apns_cli.read_until(key="Exit this menu", timeout_seconds=30)

            self.log.info(f"Entered boot menu. Select boot options")
            self.output = self.write_console_interaction_command_read_until(
                apns_cli=apns_cli,
                command="2",
                expected_key="Internal UEFI Shell",
                timeout=60,
            )

            self.log.info(f"Booting UEFI Interactive Shell")
            self.output = self.write_console_interaction_command_read_until(
                apns_cli=apns_cli,
                command="3",
                expected_key="UEFI Interactive Shell",
                timeout=180,
            )

            self.log.info(f"Press ESC to enter UEFI shell")
            command = "\x1b"  # ESC key
            for i in range(10):
                self.write_console_interaction_command(
                    apns_cli=apns_cli, command=command
                )
                time.sleep(1)

            self.output = self.write_console_interaction_command_read_until(
                apns_cli=apns_cli, command="\r\n", expected_key="Shell>", timeout=60
            )

            self.log.info(f"Boot to UEFI is successful: output = {self.output}")

            return True

        except TimeoutError:
            self.log.info("UEFI Shell is not up")

        return False

    def change_mission_mode_setting(self, apns_cli: Any, enable: bool = True) -> bool:

        assert apns_cli is not None

        if enable is True:
            self.log.info(f"Turn on MissionMode")
            MISSION_GUID = "9b35e482-22d7-47b1-ae6f-6a634cc01f87"
            CMD = "setvar"
            CMD_ARGS = [
                "MissionMode",
                "-guid",
                MISSION_GUID,
                "-bs",
                "-rt",
                "-nv",
                "=01",
            ]
            self.execute_uefi_command(
                apns_cli=apns_cli, command=CMD, command_args=CMD_ARGS
            )

            CMD = "setvar"
            CMD_ARGS = ["MissionMode", "-guid", MISSION_GUID]
            self.execute_uefi_command(
                apns_cli=apns_cli, command=CMD, command_args=CMD_ARGS
            )
            self.output = apns_cli.read_until(key="01", timeout_seconds=45)
            self.log.info(f"MissionMode is enable")
        else:
            self.log.info(f"Turn off MissionMode")
            MISSION_GUID = "9b35e482-22d7-47b1-ae6f-6a634cc01f87"
            CMD = "setvar"
            CMD_ARGS = [
                "MissionMode",
                "-guid",
                MISSION_GUID,
                "-bs",
                "-rt",
                "-nv",
                "=00",
            ]
            self.execute_uefi_command(
                apns_cli=apns_cli, command=CMD, command_args=CMD_ARGS
            )

            CMD = "setvar"
            CMD_ARGS = ["MissionMode", "-guid", MISSION_GUID]
            self.execute_uefi_command(
                apns_cli=apns_cli, command=CMD, command_args=CMD_ARGS
            )
            self.output = apns_cli.read_until(key="00", timeout_seconds=45)
            self.log.info(f"MissionMode is disable")

        return True

    def write_console_interaction_command_read_until(
        self,
        apns_cli: Any,
        command: str,
        expected_key: str = "",
        timeout: int = 1,
    ) -> str:
        """
        Execute UEFI command and return the output
        :param apns_cli:            The channel (e.g., serial or SSH) to write the command to.
        :param command:             Command to execute
        :param expected_key:        Expected key in the output
        :param timeout:             Timeout in seconds for reading the output
        :return:                    Output string
        """

        assert apns_cli is not None

        if not apns_cli.is_open():  # type: ignore
            apns_cli.open()  # type: ignore

        apns_cli.clear_buffer()
        apns_cli.write_line(write_string=command)

        self.output = apns_cli.read_until(key=expected_key, timeout_seconds=timeout)

        return self.output

    def write_console_interaction_command(
        self,
        apns_cli: Any,
        command: str,
    ) -> None:
        """
        Execute UEFI command and return the output
        :param apns_cli:            The channel (e.g., serial or SSH) to write the command to.
        :param command:             Command to execute
        :param expected_key:        Expected key in the output
        :param timeout:             Timeout in seconds for reading the output
        :return:                    Output string
        """

        assert apns_cli is not None

        if not apns_cli.is_open():  # type: ignore
            apns_cli.open()  # type: ignore

        apns_cli.clear_buffer()
        apns_cli.write(write_string=command)

    def execute_uefi_command(
        self,
        apns_cli: Any,
        command: str,
        command_args: list[str],
    ) -> None:
        """
        Execute a UEFI command and return the output.
        Args:
            apns_cli: The channel (e.g., serial or SSH) to write the command to.
            command: str: The command to execute.
            command_args: list[str]: The command arguments.
            expected_key: str: The expected key in the output.
            timeout: int: The timeout in seconds.
        Returns:
            tuple: The return code, stdout, and stderr.
            Incase of Timeout throws an exception.

        Note:
            # Step 1: Flush The buffer
            # Step 2: Write the Command
            # Step 3: First Reads the command on the prompt (Reads Shell> <cmd>)
            # Step 4: Again Read until the expected key
        """

        assert apns_cli is not None

        if not apns_cli.is_open():  # type: ignore
            apns_cli.open()  # type: ignore

        full_command = f"{command} {' '.join(command_args)}\r\r"  # Appending with \r for last prompt
        self.log.info(f"Executing UEFI Command: {repr(full_command)}")
        self.log.info("==== Buffer ====")
        apns_cli.clear_buffer()

        self.log.info("================")
        self.log.info("Sending command to UEFI shell...in chunks")
        self._send_command_in_chunks(apns_cli, full_command)

    def _send_command_in_chunks(
        self, apns_cli: Any, command: str, chunk_size: int = 16
    ) -> None:
        """
        Sends a command in smaller chunks over the given communication channel.

        Args:
            apns_cli: The channel (e.g., serial or SSH) to write the command to.
            command (str): The full command to send.
            chunk_size (int): Max size of each chunk to send. Default is 16.
        """

        assert apns_cli is not None

        for i in range(0, len(command), chunk_size):
            chunk = command[i : i + chunk_size]
            apns_cli.write(write_string=chunk)
            self.log.info(f"Sent chunk: {chunk}", do_console=True)
            time.sleep(0.1)

        if not command.endswith("\n"):
            apns_cli.write(write_string="\n")  # Ensure command terminator is sent
            self.log.info("Sent command newline", do_console=True)
