# Copyright (c) Microsoft Corporation. All rights reserved.
"""
utc_sync_test.py - Python based Pythia 2.0 Test.
Validates UTC time synchronization between BMC and MCP firmware
using the 'utc time' MCP CLI command.
"""
import re
import time
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent / 'kng_pythia_libs'))
sys.path.append(str(Path(__file__).parent.parent / 'common'))

from serial_utils import SerialUtility
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest


class utc_sync_test(EchoFallsBaseTest):
    """
    UTC Sync functional test.
    Extends EchoFallsBaseTest directly following the sensor_fifo pattern.
    Uses SerialUtility for MCP commands and Pythia BMC CLI for BMC commands.
    """

    def __init__(
        self,
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        host_config: Path | str = None,
        name: str = "utc_sync_test",
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
        self.serial_util = None
        self.bmc_cli = None

    # ── Setup / Teardown ──

    def setup_dut(self) -> bool:
        """Setup the DUT for testing."""
        try:
            self.log.info("Setting up DUT...")
            self.dut.setup()

            if self.dut.get_dut_type() == DeviceType.BIGFPGA:
                self.log.warning(
                    "Device type is bigFPGA. Performing an additional OOB reset ..."
                )
                KngPythiaTestSetup.fpga_oob_reset(self.log)

            # Open SCP and MCP channels
            self.core_com_channel_scp = (
                self.dut.mb.node_0.soc.primary_die.scp
                .channel_manager.get_current_channel()
            )
            self.core_com_channel_mcp = (
                self.dut.mb.node_0.soc.primary_die.mcp
                .channel_manager.get_current_channel()
            )

            self.serial_util = SerialUtility(
                scp_channel=self.core_com_channel_scp,
                mcp_channel=self.core_com_channel_mcp,
                logger=self.log,
                dut=self.dut,
            )

            # Wait for SCP + MCP heartbeat
            if not self.serial_util.wait_for_scp_mcp_heartbeat():
                self.log.error(
                    "Failed to receive initial SCP-MCP heartbeat during setup"
                )
                return False

            # Open BMC CLI for timedatectl / date commands
            self.bmc_cli = self.dut.mb.node_0.dcscm.bmc.cli
            self.bmc_cli.open()
            if not self.bmc_cli.is_open():
                self.log.error("Failed to open BMC CLI")
                return False
            self.log.info("BMC CLI opened successfully")

            return True
        except Exception as e:
            self.log.error(f"Error during DUT setup: {str(e)}")
            return False

    def teardown_dut(self) -> bool:
        """Teardown the DUT after testing."""
        try:
            self.log.info("Tearing down DUT...")
            if self.bmc_cli and self.bmc_cli.is_open():
                self.bmc_cli.close()
            self.dut.teardown()
            time.sleep(30)
            return True
        except Exception as e:
            self.log.error(f"Error during DUT teardown: {str(e)}")
            return False

    # ── BMC helper ──

    def _bmc_execute_command(self, command, timeout=60):
        """
        Execute a command on BMC CLI.
        Returns (return_code, stdout, stderr).
        """
        if not self.bmc_cli.is_open():
            self.log.warning("BMC CLI was closed, attempting to re-open...")
            self.bmc_cli.open()
            if not self.bmc_cli.is_open():
                self.log.error("Failed to re-open BMC CLI")
                return -1, "", "BMC CLI could not be re-opened"
        try:
            self.log.info(f"BMC executing: {command}")
            result, stdout, stderr = self.bmc_cli.execute_command(
                command=command, command_args=[], timeout_secs=timeout
            )
            return result, stdout, stderr
        except Exception as e:
            self.log.error(f"BMC command exception: {e}")
            return -1, "", str(e)

    # ── Test Case ──

    def testcase1_verify_utc_sync(self):
        """
        End-to-end UTC sync verification:
        1. Confirm BMC NTP is synchronized via timedatectl
        2. Get BMC epoch time as reference
        3. Execute 'utc time' on MCP CLI
        4. Validate MCP returns a real epoch timestamp (not uptime ticks)
        5. Compare MCP and BMC times — drift must be within tolerance
        """
        self.log.info("!Executing Test Function: testcase1_verify_utc_sync")

        # --- Step 1: Verify BMC NTP is active and synchronized ---
        step1_cmd = "timedatectl show --property=NTPSynchronized --value"
        self.log.info("Step 1: Checking BMC NTP synchronization status")
        self.log.info(f"  [BMC CMD] {step1_cmd}")
        result, stdout, stderr = self._bmc_execute_command(command=step1_cmd)
        self.log.info(
            f"  [BMC OUT] exit={result}, "
            f"stdout='{stdout.strip()}', stderr='{stderr.strip()}'"
        )
        if result != 0:
            self.log.error("Failed to run timedatectl on BMC")
            return False

        ntp_synced = stdout.strip().lower()
        if "yes" not in ntp_synced:
            self.log.error(f"BMC NTP is not synchronized: {ntp_synced}")
            return False
        self.log.info(f"  [RESULT] BMC NTP synchronized: {ntp_synced}")

        # --- Step 2: Get BMC epoch time (ms) as reference ---
        step2_cmd = "date +%s000"
        self.log.info("Step 2: Getting BMC epoch time as reference")
        self.log.info(f"  [BMC CMD] {step2_cmd}")
        result, stdout, stderr = self._bmc_execute_command(command=step2_cmd)
        self.log.info(
            f"  [BMC OUT] exit={result}, "
            f"stdout='{stdout.strip()}', stderr='{stderr.strip()}'"
        )
        if result != 0:
            self.log.error("Failed to get BMC epoch time")
            return False

        try:
            bmc_epoch_ms = int(stdout.strip())
        except ValueError:
            self.log.error(
                f"Could not parse BMC epoch time: '{stdout.strip()}'"
            )
            return False
        self.log.info(f"  [RESULT] BMC epoch time (ms): {bmc_epoch_ms}")

        # --- Step 3: Execute 'utc time' on MCP CLI ---
        step3_cmd = "utc time"
        self.log.info("Step 3: Executing 'utc time' on MCP CLI")
        self.log.info(f"  [MCP CMD] {step3_cmd}")
        mcp_response = self.serial_util.run_command_on_mcp(
            command=step3_cmd,
            read_until_key="Ok",
        )
        self.log.info(f"  [MCP OUT] response='{mcp_response}'")
        if mcp_response is None:
            self.log.error("MCP 'utc time' command failed or timed out")
            return False

        match = re.search(
            r"Current UTC Time \(ms since epoch\):\s*(\d+)", mcp_response
        )
        if not match:
            self.log.error(
                f"Could not parse MCP UTC response: '{mcp_response}'"
            )
            return False

        mcp_epoch_ms = int(match.group(1))
        self.log.info(
            f"  [RESULT] MCP UTC time (ms since epoch): {mcp_epoch_ms}"
        )

        # --- Step 4: Validate MCP time is a real epoch value ---
        # Jan 1 2025 00:00:00 UTC in ms — below this is uptime, not epoch
        min_valid_epoch_ms = 1735689600000
        self.log.info(
            f"Step 4: Validating MCP time is real epoch "
            f"(must be > {min_valid_epoch_ms}, got {mcp_epoch_ms})"
        )
        if mcp_epoch_ms < min_valid_epoch_ms:
            self.log.error(
                f"MCP UTC time ({mcp_epoch_ms} ms) appears to be uptime "
                f"ticks, not a real epoch timestamp. "
                f"UTC sync from BMC may not be established."
            )
            return False
        self.log.info("  [RESULT] MCP time is a valid epoch timestamp")

        # --- Step 5: Compare MCP time to BMC time ---
        step5_cmd = "date +%s000"
        self.log.info("Step 5: Comparing MCP time to BMC time")
        self.log.info(f"  [BMC CMD] {step5_cmd}")
        result, stdout, stderr = self._bmc_execute_command(command=step5_cmd)
        self.log.info(
            f"  [BMC OUT] exit={result}, "
            f"stdout='{stdout.strip()}', stderr='{stderr.strip()}'"
        )
        if result != 0:
            self.log.error("Failed to get fresh BMC epoch time for comparison")
            return False

        try:
            bmc_now_ms = int(stdout.strip())
        except ValueError:
            self.log.error(
                f"Could not parse fresh BMC epoch time: '{stdout.strip()}'"
            )
            return False
        drift_ms = abs(mcp_epoch_ms - bmc_now_ms)

        # NTP sync polls every 5s — allow 10s tolerance for execution delays
        max_drift_ms = 10000
        self.log.info(
            f"  [RESULT] BMC={bmc_now_ms} ms, MCP={mcp_epoch_ms} ms, "
            f"drift={drift_ms} ms, tolerance={max_drift_ms} ms"
        )

        if drift_ms > max_drift_ms:
            self.log.error(
                f"UTC drift {drift_ms} ms exceeds "
                f"{max_drift_ms} ms threshold"
            )
            return False

        self.log.info(
            f"UTC sync verified: drift {drift_ms} ms is within "
            f"{max_drift_ms} ms tolerance"
        )
        return True
