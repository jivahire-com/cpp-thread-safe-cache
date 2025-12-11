"""
PLDM API Library
Set of common functions across different test cases and test flows

Copyright (C) Microsoft Corporation. All rights reserved.
Confidential and Proprietary.
"""

import os
import re
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "kng_pythia_libs"))

from kng_pythia_test_setup import KngPythiaTestSetup
from library.pldm_tests.pldm_spec_parser import pldm_spec_parser
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
from pathlib import Path
from RscmHelperLibrary import RscmHelperLibrary
from collections import defaultdict


class pldm_common(EchoFallsBaseTest):
    """
    :param name:                Name of the test case
    :param number:              ADO Number of the test case
    :param workspace_config:    Path to the workspace config file
    :param default_log_home:    Path for log storage
    :param fw_payload_path:     Path to the recipe payload
    :param dut_platform:        Platform used during the test case execution
    :param host_config:         Path to the host config file/directory
    :param host_name:           Name of the host to find the host config file, if host_config is a directory. Defaults to None
    """

    def __init__(
        self,
        name: str,
        number: str = "NaN",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = None,
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
        self.mctp_eids = []
        self.creds = None
        self.bmc_cli = None
        self.core_scp_channel = None
        self.core_mcp_channel = None
        self.pldm_spec = None
        self.delay_5_minutes = 300
        self.delay_15_minutes = 900

    def setup(self):
        # Step 1: DUT setup
        self.dut.setup()

        # Step 2: Open MCP channel and BMC client channel
        self.core_mcp_channel = (
            self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
        )
        self.core_mcp_channel.open()
        assert self.core_mcp_channel.is_open()

        self.bmc_cli = self.dut.mb.node_0.dcscm.bmc.cli
        self.bmc_cli.open()
        assert self.bmc_cli.is_open()
        
        # Step 3: Set BMC UART MUX to MCP for RVP
        if self.dut.get_dut_type() == DeviceType.RVP:
            cred_path = os.environ.get("SECURE_FILE_PATH")
            self.creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
            self.rscm_helper = RscmHelperLibrary(
                rm_host=self.host_config.rack_scm.host,
                bmc_host=self.dut.mb.node_0.dcscm.bmc.ip,
                rm_user=self.creds["RM_USER"],
                rm_password=self.creds["RM_PASSWORD"],
                bmc_user=self.creds["BMC_USER"],
                bmc_password=self.creds["BMC_PASSWORD"],
                node=self.host_config.node_id,
            )
            self.rscm_helper.set_bmc_uart_mux_mcp(self.bmc_cli)

        # Step 4: Perform reset based on DUT type
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning(
                "Device type is bigFPGA. Performing an additional OOB reset ..."
            )
            KngPythiaTestSetup.fpga_oob_reset(self.log)
        elif self.dut.get_dut_type() == DeviceType.RVP:
            self.log.warning("Device type is RVP. Performing SoC Reset ...")
            self.rscm_helper.rscm_soc_reset()

        # Step 5: wait for MCP boot complete
        mcp_boot_status = self._check_mcp_boot_complete(timeout=self.delay_15_minutes)
        if mcp_boot_status is False:
            self.log.info("MCP boot check failed during setup")
            return False
        
        # Step 6: Wait for PLDM and MCTPD services to be active
        pldm_status = self._wait_for_service_status(
            service_str="xyz.openbmc_project.pldmd.service",
            expect_active=True,
            timeout=60,
        )
        if pldm_status is False:
            self.log.info("PLDM service is not active after waiting period")
            return False

        mctpd_status = self._wait_for_service_status(
            service_str="xyz.openbmc_project.mctpd@kernel.service",
            expect_active=True,
            timeout=60,
        )
        if mctpd_status is False:
            self.log.info("MCTPD service is not active after waiting period")
            return False

        result, _, _ = self._bmc_execute_command(
            command="systemctl stop xyz.openbmc_project.pldmd.service",
            sudo_mode=True,
        )
        if result != 0:
            self.log.info("Failed to stop pldmd service on BMC")
            return False

        time.sleep(30)

        # Step 7: Load PLDM specification data and get MCP EID
        self.pldm_spec = pldm_spec_parser()
        self.pldm_spec.load_spec_data("pldm_spec_data.json")
        mctp_id_status = self._pldm_get_mctp_id()
        if mctp_id_status is False:
            self.log.error("Failed to retrieve MCTP IDs")
            return False
        return True

    def teardown(self):
        result, _, _ = self._bmc_execute_command(
            command="systemctl start xyz.openbmc_project.pldmd.service",
            sudo_mode=True,
        )
        if result != 0:
            self.log.info("Failed to start pldmd service on BMC")
            return False

        if not self.bmc_cli.is_open():
            self.bmc_cli.open()
            assert self.bmc_cli.is_open()
        if self.dut.get_dut_type() == DeviceType.RVP:
            self.rscm_helper.set_bmc_uart_mux_scp(self.bmc_cli)
        self.bmc_cli.close()
        self.dut.teardown()
        time.sleep(10)
        return True

    def _backoff_delay(
        self,
        attempt: int,
        base_delay: float = 10.0,
        max_delay: float = 300.0,
        max_retries: int = 10,
    ):
        """
        Exponential backoff delay calculation
        """
        growth_factor = (max_delay / base_delay) ** (1.0 / (max_retries - 1))
        delay = base_delay * (growth_factor ** (attempt - 1))
        return min(delay, max_delay)

    def _bmc_execute_command(
        self, command, timeout=180, sudo_mode=False, max_retries=10, retry_delay=10
    ):
        """
        Execute a command on BMC with retry logic (exponential backoff delay).

        :param command: Command to execute
        :param timeout: Timeout per attempt in seconds (default: 180s)
        :param sudo_mode: Whether to run command with sudo
        :param max_retries: Maximum retry attempts (default: 10)
        :param retry_delay: Base delay between retries in seconds (default: 10s)
        """
        cmd_str = command
        if not self.bmc_cli.is_open():
            self.bmc_cli.open()
            assert self.bmc_cli.is_open()

        if sudo_mode:
            try:
                sudo_password = self.creds["BMC_PASSWORD"]
                cmd_str = f"echo {sudo_password} | sudo -S su -c '{command}'"
            except (TypeError, KeyError) as e:
                self.log.info(f"Failed to get BMC sudo password: {e}")
                self.bmc_cli.close()
                raise AssertionError("Missing or invalid BMC password") from e

        attempt = 0
        while attempt < max_retries:
            attempt += 1
            try:
                self.log.info(
                    f"Executing BMC command (Attempt {attempt}/{max_retries}): {cmd_str}"
                )
                result, stdout, stderr = self.bmc_cli.execute_command(
                    command=cmd_str, command_args=[], timeout_secs=timeout
                )

                if result == 0 and not self._pldm_failed(stdout, stderr):
                    self.log.info("Command executed successfully.")
                    break
                else:
                    self.log.warning(
                        f"Command failed with result={result}. stderr={stderr}"
                    )

            except TimeoutError as e:
                self.log.warning(
                    f"TimeoutError on attempt {attempt}/{max_retries}: {e}"
                )
                if attempt == max_retries:
                    self.log.error("Max retries reached. Command failed permanently.")
                    self.bmc_cli.close()
                    return -1, "", f"TimeoutError: {e}"

            backoff_delay = self._backoff_delay(
                attempt,
                base_delay=retry_delay,
                max_delay=self.delay_5_minutes,
                max_retries=max_retries,
            )
            self.log.info(f"Retrying after {backoff_delay} seconds...")
            time.sleep(backoff_delay)

        # MCP health check after command
        mcp_health_status = self._check_mcp_health()
        if mcp_health_status is False:
            self.log.info("MCP health check failed after executing command")
            result = -1

        self.bmc_cli.close()
        return result, stdout, stderr

    def _check_mcp_boot_complete(self, timeout):
        try:
            self.core_mcp_channel.read_until(
                key="StartupShutdownSvc::LocalCoreSyncStageRemoteEnd",
                timeout_seconds=timeout,
            )
            self.log.info(
                f"Boot message 'StartupShutdownSvc::LocalCoreSyncStageRemoteEnd' acknowledged from MCP"
            )
        except TimeoutError as e:
            print(f"A TimeoutError occurred while reading a boot message from MCP: {e}")
            return False
        return True

    def _check_mcp_health(self, waiting_time_seconds=20):
        try:
            self.core_mcp_channel.read_until(
                key="MCP_MAIN::McpHeartBeat",
                timeout_seconds=waiting_time_seconds,
            )
            self.log.info(
                f"Health message 'MCP_MAIN::McpHeartBeat' acknowledged from MCP"
            )
        except TimeoutError as e:
            print(f"A TimeoutError occurred while checking the health of the MCP: {e}")
            return False
        return True

    def _mcp_execute_command_until(self, command, key):
        cmd_str = command
        try:
            result, stdout, stderr = self.core_mcp_channel.execute_command_until(
                command=cmd_str, command_args=[], key=key, timeout_secs=60
            )
            if result != 0:
                self.log.info(f"~$ MCP failed to execute command: {cmd_str}")
            return result, stdout, stderr
        except TimeoutError:
            self.log.warning(
                f"~$ MCP, Timeout occurred while waiting for key '{key}' in MCP command: {cmd_str}. Test flow will continue."
            )
            return 1, "", "TimeoutError"
        except Exception as e:
            self.log.warning(
                f"~$ MCP, Unexpected error during MCP command '{cmd_str}': {e}. Test flow will continue."
            )
            return 1, "", str(e)

    def _pldm_failed(self, stdout: str, stderr: str):
        """
        Return True if output contains signatures that indicate PLDM failure
        even when the shell exit code is 0.
        """
        text = (stdout + "\n" + stderr).lower()
        error_patterns = [
            "error: failure to read response",
            "failed to receive",
            "input/output error",
        ]
        return any(p in text for p in error_patterns)

    def _pldm_get_mctp_id(self):
        """
        Get MCTP EIDs from BMC
        """
        self.log.info("Execute busctl tree command")

        result, stdout, _ = self._bmc_execute_command(
            "busctl tree xyz.openbmc_project.MCTP-kernel"
        )
        if result != 0:
            return False

        self.log.info("Execute busctl tree command successfully")

        matches = re.findall(r"(\/.*\/device\/\d+)", stdout)
        num_matches = len(matches)
        self.log.info(f"Number of matches: {num_matches}")

        for index, match in enumerate(matches):
            self.log.info(f"\n\nMctp Device\n {index}: {match}")
            result, location_code, _ = self._bmc_execute_command(
                f"busctl introspect xyz.openbmc_project.MCTP-kernel {match}"
            )
            if result != 0:
                continue
            location_code_match = re.findall(r"property.*MCP", location_code)
            if location_code_match:
                self.log.info(f"Found MCP {match}")
                mctp_id_match = re.findall(r"(\d+)$", match)
                if not mctp_id_match:
                    self.log.error("No MCTP EID found for MCP")
                    return False
                self.mctp_eids.append(int(mctp_id_match[0]))

        return True

    def _wait_for_service_status(
        self,
        service_str: str = "",
        expect_active: bool = True,
        timeout: int = 60,
        interval: float = 0.5,
    ):
        """
        Poll until service becomes active/inactive or until the timeout expires.
        If expect_active is True, wait for service to become active.
        If expect_active is False, wait for service to become inactive.
        """
        deadline = time.monotonic() + timeout
        last_state = ""

        while time.monotonic() < deadline:
            _, stdout, _ = self._bmc_execute_command(
                command=f"systemctl is-active {service_str}",
                sudo_mode=True,
            )
            state = stdout.strip()
            last_state = state

            if expect_active is True:
                if state == "active":
                    self.log.info(f"{service_str} service is active.")
                    return True
            else:
                if state != "active":
                    self.log.info(f"{service_str} service is inactive.")
                    return True

            time.sleep(interval)

        msg = ""
        if expect_active:
            msg = f"{service_str} service did not become active within {timeout}s (last state: '{last_state}')"
        else:
            msg = f"{service_str} service did not become inactive within {timeout}s (last state: '{last_state}')"
        self.log.warning(msg)
        return False
