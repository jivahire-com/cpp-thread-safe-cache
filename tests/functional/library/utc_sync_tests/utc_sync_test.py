# Copyright (c) Microsoft Corporation. All rights reserved.
"""
utc_sync_test.py - Python based Pythia 2.0 Test.
Validates UTC time synchronization between BMC and MCP firmware
using the 'utc time' MCP CLI command.
"""
import time
import sys
import re
from pathlib import Path
from datetime import datetime, timezone

sys.path.append(str(Path(__file__).parent.parent / "kng_pythia_libs"))
sys.path.append(str(Path(__file__).parent.parent / "common"))
sys.path.append(str(Path(__file__).parent.parent / "pldm_tests"))

from pldm_common import pldm_common


class utc_sync_test(pldm_common):
    """
    UTC Sync functional test.
    Inherits from pldm_common to leverage BMC/MCP command execution and PLDM checks.
    """

    def __init__(
        self,
        workspace_config: Path | str | None = None,
        default_log_home: str | None = None,
        fw_payload_path: str | None = None,
        host_config: Path | str | None = None,
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

    # ── Setup / Teardown ──

    def setup_dut(self) -> bool:
        """Setup the DUT for testing using pldm_common setup with PLDM active."""
        try:
            # pldm_common.setup() handles:
            #   DUT init, BMC/MCP/SCP channels, MCP boot check, PLDM/MCTPD wait
            result = self.setup(keep_pldm_service_active=True)
            if not result:
                self.log.error("pldm_common setup failed")
                return False

            # Verify MCP channel is open (same pattern as pldm_sensor_test)
            if not self.core_mcp_channel.is_open():
                self.log.error("MCP channel not open after setup")
                return False

            return True
        except Exception as e:
            self.log.error(f"Error during DUT setup: {e}", exc_info=True)
            return False

    def teardown_dut(self) -> bool:
        """Teardown the DUT after testing."""
        try:
            self.teardown()
            time.sleep(30)
            return True
        except Exception as e:
            self.log.error(f"Error during DUT teardown: {e}", exc_info=True)
            return False

    # ── Helper Methods ──

    def _get_mcp_utc_time(self) -> str:
        """
        Execute 'utc time' on MCP CLI via inherited _mcp_execute_command_until.

        Returns:
            MCP output string, or empty string on failure.
        """
        cmd = "utc time"
        key = "Ok"
        result, stdout, stderr = self._mcp_execute_command_until(command=cmd, key=key)
        self.log.info(f"[RAW_COMMAND_OUTPUT] cmd='{cmd}', key='{key}'")
        self.log.info(
            f"[RAW_COMMAND_OUTPUT] exit={result}, "
            f"stdout='{stdout}', stderr='{stderr}'"
        )
        if result != 0:
            self.log.error(
                f"[PARSED_OUTPUT] MCP 'utc time' failed: "
                f"exit={result}, stderr='{stderr}'"
            )
            return ""
        stripped = stdout.strip()
        self.log.info(f"[PARSED_OUTPUT] MCP utc time: '{stripped}'")
        return stripped

    def _get_bmc_timedatectl(self) -> str:
        """
        Execute 'timedatectl' on BMC and return raw output.

        Returns:
            Raw BMC timedatectl output string, or empty string on failure.
        """
        cmd = "timedatectl"
        result, stdout, stderr = self._bmc_execute_command(command=cmd)
        self.log.info(f"[RAW_COMMAND_OUTPUT] cmd='{cmd}'")
        self.log.info(
            f"[RAW_COMMAND_OUTPUT] exit={result}, "
            f"stdout='{stdout}', stderr='{stderr}'"
        )
        if result != 0:
            self.log.error(
                f"[PARSED_OUTPUT] timedatectl failed: "
                f"exit={result}, stderr='{stderr}'"
            )
            return ""
        stripped = stdout.strip()
        self.log.info(f"[PARSED_OUTPUT] BMC timedatectl:\n{stripped}")
        return stripped

    def _check_pldm_service_active(self) -> bool:
        """
        Check if PLDM service is running.

        Returns:
            True if PLDM service is active, False otherwise.
        """
        return self._wait_for_service_status(
            service_str="xyz.openbmc_project.pldmd.service",
            expect_active=True,
            timeout=10,
        )

    def _parse_mcp_timestamp(self, mcp_output: str) -> int | None:
        """
        Extract milliseconds since epoch from MCP 'utc time' output.

        Args:
            mcp_output: Raw output from 'utc time' command

        Returns:
            Timestamp in milliseconds, or None if parsing fails
        """
        match = re.search(r"Current UTC Time \(ms since epoch\):\s*(\d+)", mcp_output)
        if match:
            return int(match.group(1))
        return None

    def _parse_bmc_timestamp(self, bmc_output: str) -> int | None:
        """
        Extract seconds since epoch from BMC 'timedatectl' output.

        Args:
            bmc_output: Raw output from 'timedatectl' command

        Returns:
            Timestamp in seconds, or None if parsing fails
        """
        # Extract "Local time: Wed 2026-02-25 10:01:11 UTC"
        match = re.search(
            r"Local time:\s+(\w+)\s+(\d{4})-(\d{2})-(\d{2})\s+(\d{2}):(\d{2}):(\d{2})\s+UTC",
            bmc_output,
        )
        if match:
            year, month, day, hour, minute, second = map(int, match.groups()[1:])
            dt = datetime(year, month, day, hour, minute, second, tzinfo=timezone.utc)
            return int(dt.timestamp())
        return None

    def _compare_times(
        self, mcp_ms: int, bmc_sec: int, tolerance_sec: float = 10.0
    ) -> tuple[bool, str]:
        """
        Compare MCP and BMC timestamps with tolerance for command execution delay.

        Args:
            mcp_ms: MCP timestamp in milliseconds since epoch
            bmc_sec: BMC timestamp in seconds since epoch
            tolerance_sec: Maximum allowed difference in seconds (default 10.0)

        Returns:
            Tuple of (match_ok: bool, description: str)
        """
        mcp_sec = mcp_ms / 1000.0
        diff_sec = abs(mcp_sec - bmc_sec)

        match_ok = diff_sec <= tolerance_sec
        desc = f"MCP={mcp_sec:.3f}s, BMC={bmc_sec:.3f}s, Diff={diff_sec:.3f}s"

        return match_ok, desc

    # ── Test Case ──

    def testcase1_capture_utc_times(self) -> dict:
        """
        Capture UTC time from MCP and BMC twice with 40-second delay.

        Steps:
        1. Confirm PLDM service is running
        2. Capture MCP 'utc time' output
        3. Capture BMC 'timedatectl' output
        4. Wait 40 seconds
        5. Capture MCP 'utc time' output again
        6. Capture BMC 'timedatectl' output again
        7. Compare timestamps from each capture
        8. Return all captured outputs and comparison results for validation

        Returns:
            dict with keys:
                - pldm_active: bool
                - mcp_time_1: str (first MCP utc time output)
                - bmc_time_1: str (first BMC timedatectl output)
                - mcp_time_2: str (second MCP utc time output after 40s)
                - bmc_time_2: str (second BMC timedatectl output after 40s)
                - mcp_timestamp_1_ms: int or None (parsed MCP timestamp)
                - bmc_timestamp_1_sec: int or None (parsed BMC timestamp)
                - mcp_timestamp_2_ms: int or None (parsed MCP timestamp)
                - bmc_timestamp_2_sec: int or None (parsed BMC timestamp)
                - time_match_1: bool (first samples match within tolerance)
                - time_match_2: bool (second samples match within tolerance)
                - success: bool (True if all captures and comparisons succeeded)
        """
        self.log.info("=" * 60)
        self.log.info("UTC TIME CAPTURE TEST")
        self.log.info("=" * 60)

        result = {
            "pldm_active": False,
            "mcp_time_1": "",
            "bmc_time_1": "",
            "mcp_time_2": "",
            "bmc_time_2": "",
            "mcp_timestamp_1_ms": None,
            "bmc_timestamp_1_sec": None,
            "mcp_timestamp_2_ms": None,
            "bmc_timestamp_2_sec": None,
            "time_match_1": False,
            "time_match_2": False,
            "success": False,
        }

        # Step 1: Confirm PLDM is running
        self.log.info("Step 1: Checking PLDM service status...")
        pldm_active = self._check_pldm_service_active()
        result["pldm_active"] = pldm_active
        if not pldm_active:
            self.log.error("[FAIL] PLDM service is not active")
            return result
        self.log.info("[PASS] PLDM service is active")

        # Step 2: First capture - MCP UTC time
        self.log.info("Step 2: Capturing MCP 'utc time' (first sample)...")
        mcp_time_1 = self._get_mcp_utc_time()
        result["mcp_time_1"] = mcp_time_1
        if not mcp_time_1:
            self.log.error("[FAIL] Failed to get MCP UTC time (first sample)")
            return result
        # Step 3: First capture - BMC timedatectl
        self.log.info("Step 3: Capturing BMC 'timedatectl' (first sample)...")
        bmc_time_1 = self._get_bmc_timedatectl()
        result["bmc_time_1"] = bmc_time_1
        if not bmc_time_1:
            self.log.error("[FAIL] Failed to get BMC timedatectl (first sample)")
            return result

        # Step 4: Wait 40 seconds
        self.log.info("Step 4: Waiting 40 seconds before second capture...")
        time.sleep(40)

        # Step 5: Second capture - MCP UTC time
        self.log.info("Step 5: Capturing MCP 'utc time' (second sample)...")
        mcp_time_2 = self._get_mcp_utc_time()
        result["mcp_time_2"] = mcp_time_2
        if not mcp_time_2:
            self.log.error("[FAIL] Failed to get MCP UTC time (second sample)")
            return result
        # Step 6: Second capture - BMC timedatectl
        self.log.info("Step 6: Capturing BMC 'timedatectl' (second sample)...")
        bmc_time_2 = self._get_bmc_timedatectl()
        result["bmc_time_2"] = bmc_time_2
        if not bmc_time_2:
            self.log.error("[FAIL] Failed to get BMC timedatectl (second sample)")
            return result

        # Step 7: Parse timestamps and compare
        self.log.info("Step 7: Parsing and comparing timestamps...")

        # Parse first sample
        mcp_ts_1 = self._parse_mcp_timestamp(mcp_time_1)
        bmc_ts_1 = self._parse_bmc_timestamp(bmc_time_1)
        result["mcp_timestamp_1_ms"] = mcp_ts_1
        result["bmc_timestamp_1_sec"] = bmc_ts_1

        if mcp_ts_1 is not None and bmc_ts_1 is not None:
            match_1, desc_1 = self._compare_times(mcp_ts_1, bmc_ts_1)
            result["time_match_1"] = match_1
            status_1 = "[PASS]" if match_1 else "[FAIL]"
            self.log.info(f"  {status_1} First sample: {desc_1}")
        else:
            self.log.error(
                f"  [FAIL] Could not parse first sample: "
                f"mcp={mcp_ts_1}, bmc={bmc_ts_1}"
            )
            result["time_match_1"] = False
            return result

        # Parse second sample
        mcp_ts_2 = self._parse_mcp_timestamp(mcp_time_2)
        bmc_ts_2 = self._parse_bmc_timestamp(bmc_time_2)
        result["mcp_timestamp_2_ms"] = mcp_ts_2
        result["bmc_timestamp_2_sec"] = bmc_ts_2

        if mcp_ts_2 is not None and bmc_ts_2 is not None:
            match_2, desc_2 = self._compare_times(mcp_ts_2, bmc_ts_2)
            result["time_match_2"] = match_2
            status_2 = "[PASS]" if match_2 else "[FAIL]"
            self.log.info(f"  {status_2} Second sample: {desc_2}")
        else:
            self.log.error(
                f"  [FAIL] Could not parse second sample: "
                f"mcp={mcp_ts_2}, bmc={bmc_ts_2}"
            )
            result["time_match_2"] = False
            return result

        # Determine overall success based on both samples matching
        result["success"] = result["time_match_1"] and result["time_match_2"]

        self.log.info("=" * 60)
        if result["success"]:
            self.log.info("[PASS] UTC times are synchronized (both samples matched)")
        else:
            self.log.error("[FAIL] UTC time synchronization failed")
            if not result["time_match_1"]:
                self.log.error("  - First sample: times do not match within tolerance")
            if not result["time_match_2"]:
                self.log.error("  - Second sample: times do not match within tolerance")
        self.log.info("=" * 60)

        return result
