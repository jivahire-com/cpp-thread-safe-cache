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
        desc = f"MCP={mcp_sec:.3f}s, BMC={bmc_sec:.3f}s, Diff={diff_sec:.3f}s (Tolerance={tolerance_sec:.3f}s)"

        return match_ok, desc

    # ── Test Case ──

    def testcase1_capture_utc_times(self) -> dict:
        """
        Capture UTC time from MCP and BMC five times with 40-second delays.

        Steps:
        1. Confirm PLDM service is running
        2. Capture MCP 'utc time' and BMC 'timedatectl' (Sample 1)
        3. Wait 40 seconds
        4. Capture MCP 'utc time' and BMC 'timedatectl' (Sample 2)
        5. Wait 40 seconds
        6. Capture MCP 'utc time' and BMC 'timedatectl' (Sample 3)
        7. Wait 40 seconds
        8. Capture MCP 'utc time' and BMC 'timedatectl' (Sample 4)
        9. Wait 40 seconds
        10. Capture MCP 'utc time' and BMC 'timedatectl' (Sample 5)
        11. Compare timestamps from all captures
        12. Return all captured outputs and comparison results for validation

        Returns:
            dict with keys:
                - pldm_active: bool
                - tolerance_sec: float (tolerance range used for comparisons)
                - mcp_time_[1-5]: str (MCP utc time outputs)
                - bmc_time_[1-5]: str (BMC timedatectl outputs)
                - mcp_timestamp_[1-5]_ms: int or None (parsed MCP timestamps)
                - bmc_timestamp_[1-5]_sec: int or None (parsed BMC timestamps)
                - time_match_[1-5]: bool (sample matches within tolerance)
                - success: bool (True if all captures and comparisons succeeded)
        """
        tolerance_sec = 10.0

        self.log.info("=" * 60)
        self.log.info("UTC TIME CAPTURE TEST (5 SAMPLES)")
        self.log.info("=" * 60)
        self.log.info(f"Tolerance Range: +/- {tolerance_sec:.1f} seconds")
        self.log.info("=" * 60)

        result = {
            "pldm_active": False,
            "tolerance_sec": tolerance_sec,
            "success": False,
        }

        # Initialize result dictionaries for 5 samples
        for i in range(1, 6):
            result[f"mcp_time_{i}"] = ""
            result[f"bmc_time_{i}"] = ""
            result[f"mcp_timestamp_{i}_ms"] = None
            result[f"bmc_timestamp_{i}_sec"] = None
            result[f"time_match_{i}"] = False

        # Step 1: Confirm PLDM is running
        self.log.info("Step 1: Checking PLDM service status...")
        pldm_active = self._check_pldm_service_active()
        result["pldm_active"] = pldm_active
        if not pldm_active:
            self.log.error("[FAIL] PLDM service is not active")
            return result
        self.log.info("[PASS] PLDM service is active")

        # Capture 5 samples with 40-second intervals
        for sample_num in range(1, 6):
            self.log.info(f"\n--- SAMPLE {sample_num}/5 ---")

            # Capture MCP UTC time
            self.log.info(f"Capturing MCP 'utc time' (sample {sample_num})...")
            mcp_time = self._get_mcp_utc_time()
            result[f"mcp_time_{sample_num}"] = mcp_time
            if not mcp_time:
                self.log.error(
                    f"[FAIL] Failed to get MCP UTC time (sample {sample_num})"
                )
                return result

            # Capture BMC timedatectl
            self.log.info(f"Capturing BMC 'timedatectl' (sample {sample_num})...")
            bmc_time = self._get_bmc_timedatectl()
            result[f"bmc_time_{sample_num}"] = bmc_time
            if not bmc_time:
                self.log.error(
                    f"[FAIL] Failed to get BMC timedatectl (sample {sample_num})"
                )
                return result

            # Parse timestamps
            mcp_ts = self._parse_mcp_timestamp(mcp_time)
            bmc_ts = self._parse_bmc_timestamp(bmc_time)
            result[f"mcp_timestamp_{sample_num}_ms"] = mcp_ts
            result[f"bmc_timestamp_{sample_num}_sec"] = bmc_ts

            # Compare timestamps
            if mcp_ts is not None and bmc_ts is not None:
                match, desc = self._compare_times(mcp_ts, bmc_ts, tolerance_sec)
                result[f"time_match_{sample_num}"] = match
                status = "[PASS]" if match else "[FAIL]"
                self.log.info(f"  {status} Sample {sample_num}: {desc}")
            else:
                self.log.error(
                    f"  [FAIL] Could not parse sample {sample_num}: "
                    f"mcp={mcp_ts}, bmc={bmc_ts}"
                )
                result[f"time_match_{sample_num}"] = False
                return result

            # Wait 40 seconds before next sample (except after last sample)
            if sample_num < 5:
                self.log.info(f"Waiting 40 seconds before sample {sample_num + 1}...")
                time.sleep(40)

        # Determine overall success - all 5 samples must match
        all_matches = all(result[f"time_match_{i}"] for i in range(1, 6))
        result["success"] = all_matches

        self.log.info("\n" + "=" * 60)
        self.log.info("TEST SUMMARY")
        self.log.info("=" * 60)
        for i in range(1, 6):
            status = "PASS" if result[f"time_match_{i}"] else "FAIL"
            self.log.info(f"  Sample {i}: {status}")
        self.log.info("=" * 60)

        if result["success"]:
            self.log.info("[PASS] UTC times are synchronized (all 5 samples matched)")
        else:
            self.log.error("[FAIL] UTC time synchronization failed")
            for i in range(1, 6):
                if not result[f"time_match_{i}"]:
                    self.log.error(
                        f"  - Sample {i}: times do not match within tolerance"
                    )
        self.log.info("=" * 60)

        return result
