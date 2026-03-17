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
from threading import Thread

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

    def _get_mcp_utc_time(self, include_timing: bool = False):
        """
        Execute 'utc time' on MCP CLI via inherited _mcp_execute_command_until.

        Returns:
            MCP output string, or empty string on failure.
            If include_timing=True, returns tuple:
                (output: str, cmd_start_wall: float, cmd_end_wall: float)
        """
        cmd = "utc time"
        key = "Ok"

        start_wall = time.time()
        result, stdout, stderr = self._mcp_execute_command_until(command=cmd, key=key)
        end_wall = time.time()

        start_utc = datetime.fromtimestamp(start_wall, timezone.utc).isoformat(
            timespec="milliseconds"
        )
        end_utc = datetime.fromtimestamp(end_wall, timezone.utc).isoformat(
            timespec="milliseconds"
        )
        elapsed = end_wall - start_wall
        self.log.info(
            f"[CMD_TIMING][MCP] START cmd='{cmd}' utc='{start_utc}' "
            f"wall={start_wall:.6f}"
        )
        self.log.info(
            f"[CMD_TIMING][MCP] END cmd='{cmd}' utc='{end_utc}' "
            f"wall={end_wall:.6f} elapsed={elapsed:.3f}s"
        )

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
            return ("", start_wall, end_wall) if include_timing else ""
        stripped = stdout.strip()
        self.log.info(f"[PARSED_OUTPUT] MCP utc time: '{stripped}'")
        return (stripped, start_wall, end_wall) if include_timing else stripped

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

    def _close_bmc_cli_if_open(self) -> None:
        """Close BMC CLI connection when open; ignore close errors."""
        try:
            if self.bmc_cli.is_open():
                self.bmc_cli.close()
        except Exception:
            pass

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

    # ── Parallel Capture Helpers ──

    def _get_bmc_time_no_health_check(
        self, close_after_command: bool = True, include_timing: bool = False
    ):
        """
        Execute 'timedatectl' on BMC WITHOUT the post-command MCP health check.

        _bmc_execute_command() calls _check_mcp_health() after every command,
        which reads core_mcp_channel (UART). Running that concurrently with
        _get_mcp_utc_time() on the same UART would corrupt the data stream.

        This helper uses bmc_cli directly (open/execute/close) to avoid that.

        Args:
            close_after_command: When True, closes bmc_cli after command.
                For parallel optimization, keep False to reuse SSH session.

        Returns:
            Raw BMC timedatectl output string, or empty string on failure.
            If include_timing=True, returns tuple:
                (output: str, cmd_start_wall: float, cmd_end_wall: float)
        """
        cmd = "timedatectl"
        try:
            if not self.bmc_cli.is_open():
                self.bmc_cli.open()
            self.log.info(f"[BMC CMD (no health check)] {cmd}")

            start_wall = time.time()
            result, stdout, stderr = self.bmc_cli.execute_command(
                command=cmd, command_args=[], timeout_secs=30
            )
            end_wall = time.time()

            start_utc = datetime.fromtimestamp(start_wall, timezone.utc).isoformat(
                timespec="milliseconds"
            )
            end_utc = datetime.fromtimestamp(end_wall, timezone.utc).isoformat(
                timespec="milliseconds"
            )
            elapsed = end_wall - start_wall
            self.log.info(
                f"[CMD_TIMING][BMC_NO_HEALTH] START cmd='{cmd}' utc='{start_utc}' "
                f"wall={start_wall:.6f}"
            )
            self.log.info(
                f"[CMD_TIMING][BMC_NO_HEALTH] END cmd='{cmd}' utc='{end_utc}' "
                f"wall={end_wall:.6f} elapsed={elapsed:.3f}s"
            )

            self.log.info(
                f"[RAW_COMMAND_OUTPUT] cmd='{cmd}', "
                f"exit={result}, stdout='{stdout.strip()[:200]}', "
                f"stderr='{stderr}'"
            )
            if close_after_command:
                self._close_bmc_cli_if_open()
            if result != 0:
                self.log.error(
                    f"BMC 'timedatectl' failed: exit={result}, stderr='{stderr}'"
                )
                return ("", start_wall, end_wall) if include_timing else ""
            stripped = stdout.strip()
            self.log.info(f"[PARSED_OUTPUT] BMC timedatectl:\n{stripped}")
            return (stripped, start_wall, end_wall) if include_timing else stripped
        except Exception as e:
            self.log.error(f"BMC 'timedatectl' exception: {e}")
            if close_after_command:
                self._close_bmc_cli_if_open()
            return ("", 0.0, 0.0) if include_timing else ""

    def _capture_times_parallel(self, close_bmc_after_command: bool = True) -> dict:
        """
        Capture MCP and BMC timestamps in parallel using threads.

        MCP is queried via UART ('utc time') and BMC via SSH ('timedatectl').
        These use independent I/O channels (UART vs SSH), so true parallel
        execution is safe — provided the BMC helper does NOT read the MCP
        UART (which _get_bmc_time_no_health_check ensures by skipping the
        MCP health check).

        Records wall-clock timestamps around each call for instrumentation.

        Returns:
            dict with keys:
                - mcp_output: str
                - bmc_output: str
                - mcp_cmd_wall_before / mcp_cmd_wall_after: float
                - bmc_cmd_wall_before / bmc_cmd_wall_after: float
        """
        capture = {
            "mcp_output": "",
            "bmc_output": "",
            "mcp_cmd_wall_before": 0.0,
            "mcp_cmd_wall_after": 0.0,
            "bmc_cmd_wall_before": 0.0,
            "bmc_cmd_wall_after": 0.0,
        }

        def _fetch_mcp():
            (
                capture["mcp_output"],
                capture["mcp_cmd_wall_before"],
                capture["mcp_cmd_wall_after"],
            ) = self._get_mcp_utc_time(include_timing=True)

        def _fetch_bmc():
            (
                capture["bmc_output"],
                capture["bmc_cmd_wall_before"],
                capture["bmc_cmd_wall_after"],
            ) = self._get_bmc_time_no_health_check(
                close_after_command=close_bmc_after_command,
                include_timing=True,
            )

        t_mcp = Thread(target=_fetch_mcp, daemon=True)
        t_bmc = Thread(target=_fetch_bmc, daemon=True)

        t_mcp.start()
        t_bmc.start()

        t_mcp.join(timeout=120)
        t_bmc.join(timeout=120)

        return capture

    # ── Test Case 2: Parallel + Instrumented ──

    def testcase2_parallel_capture_utc_times(self) -> dict:
        """
        Capture UTC time from MCP and BMC in true parallel ten times,
        with debug instrumentation.

        Both queries run on separate threads using independent I/O channels:
          - MCP: UART 'utc time' command  (returns ms since epoch)
          - BMC: SSH  'timedatectl'       (returns seconds resolution)

        The BMC helper (_get_bmc_time_no_health_check) deliberately skips
        the MCP health check to avoid UART contention with the MCP thread.

        Debug output includes wall-clock instrumentation so the capture
        overhead and overlap for each channel is visible in the log.

        Returns:
            dict with keys:
                - pldm_active: bool
                - tolerance_sec: float
                - mcp_time_[1-10]: str        (raw MCP output)
                - bmc_time_[1-10]: str        (raw BMC output)
                - mcp_timestamp_[1-10]_ms: int | None
                - bmc_timestamp_[1-10]_sec: int | None
                - time_match_[1-10]: bool
                - mcp_capture_latency_[1-10]_sec: float
                - bmc_capture_latency_[1-10]_sec: float
                - raw_diff_[1-10]_sec: float
                - corrected_diff_[1-10]_sec: float
                - success: bool
        """
        tolerance_sec = 4.0
        sample_count = 10

        self.log.info("=" * 60)
        self.log.info("UTC TIME PARALLEL CAPTURE TEST (10 SAMPLES)")
        self.log.info("=" * 60)
        self.log.info(f"Tolerance: +/- {tolerance_sec:.1f} seconds")
        self.log.info("MCP: UART 'utc time' (ms since epoch)")
        self.log.info("BMC: SSH 'timedatectl' (seconds resolution)")
        self.log.info("Mode: true parallel (MCP UART + BMC SSH, no contention)")
        self.log.info("=" * 60)

        result = {
            "pldm_active": False,
            "tolerance_sec": tolerance_sec,
            "success": False,
        }

        # Initialize per-sample result keys
        for i in range(1, sample_count + 1):
            result[f"mcp_time_{i}"] = ""
            result[f"bmc_time_{i}"] = ""
            result[f"mcp_timestamp_{i}_ms"] = None
            result[f"bmc_timestamp_{i}_sec"] = None
            result[f"time_match_{i}"] = False
            result[f"mcp_capture_latency_{i}_sec"] = None
            result[f"bmc_capture_latency_{i}_sec"] = None
            result[f"raw_diff_{i}_sec"] = None
            result[f"corrected_diff_{i}_sec"] = None

        # Step 1: Confirm PLDM is running
        self.log.info("Step 1: Checking PLDM service status...")
        pldm_active = self._check_pldm_service_active()
        result["pldm_active"] = pldm_active
        if not pldm_active:
            self.log.error("[FAIL] PLDM service is not active")
            return result
        self.log.info("[PASS] PLDM service is active")

        # Keep BMC SSH session open across all parallel samples (faster path)
        if not self.bmc_cli.is_open():
            self.bmc_cli.open()
            self.log.info("BMC fast path session opened (reused across samples)")

        # Capture 10 samples with 40-second intervals
        for sample_num in range(1, sample_count + 1):
            self.log.info("")
            self.log.info(f"--- SAMPLE {sample_num}/{sample_count} (parallel) ---")

            # Launch parallel capture
            cap = self._capture_times_parallel(close_bmc_after_command=False)

            mcp_time = cap["mcp_output"]
            bmc_time = cap["bmc_output"]
            result[f"mcp_time_{sample_num}"] = mcp_time
            result[f"bmc_time_{sample_num}"] = bmc_time

            # Compute command capture latencies (command-only windows)
            mcp_latency = cap["mcp_cmd_wall_after"] - cap["mcp_cmd_wall_before"]
            bmc_latency = cap["bmc_cmd_wall_after"] - cap["bmc_cmd_wall_before"]
            result[f"mcp_capture_latency_{sample_num}_sec"] = mcp_latency
            result[f"bmc_capture_latency_{sample_num}_sec"] = bmc_latency

            self.log.info("[DEBUG TIMING]")
            self.log.info(f"  MCP capture latency  : {mcp_latency:.3f} s")
            self.log.info(f"  BMC capture latency  : {bmc_latency:.3f} s")
            self.log.info(
                f"  MCP cmd window       : "
                f"{cap['mcp_cmd_wall_before']:.3f} -> {cap['mcp_cmd_wall_after']:.3f}"
            )
            self.log.info(
                f"  BMC cmd window       : "
                f"{cap['bmc_cmd_wall_before']:.3f} -> {cap['bmc_cmd_wall_after']:.3f}"
            )

            # Validate raw outputs
            if not mcp_time:
                self.log.error(
                    f"[FAIL] MCP 'utc time' returned empty (sample {sample_num})"
                )
                self._close_bmc_cli_if_open()
                return result
            if not bmc_time:
                self.log.error(
                    f"[FAIL] BMC 'timedatectl' returned empty (sample {sample_num})"
                )
                self._close_bmc_cli_if_open()
                return result

            # Parse timestamps
            mcp_ms = self._parse_mcp_timestamp(mcp_time)
            bmc_sec = self._parse_bmc_timestamp(bmc_time)
            result[f"mcp_timestamp_{sample_num}_ms"] = mcp_ms
            result[f"bmc_timestamp_{sample_num}_sec"] = bmc_sec

            if mcp_ms is None or bmc_sec is None:
                self.log.error(
                    f"[FAIL] Could not parse timestamps (sample {sample_num}): "
                    f"mcp_ms={mcp_ms}, bmc_sec={bmc_sec}"
                )
                result[f"time_match_{sample_num}"] = False
                self._close_bmc_cli_if_open()
                return result

            # Compute diffs
            mcp_sec_f = mcp_ms / 1000.0
            raw_diff = float(bmc_sec) - mcp_sec_f
            result[f"raw_diff_{sample_num}_sec"] = raw_diff

            # Manager-requested correction:
            # corrected = (BMC + bmc_elapsed/2) - (MCP + mcp_elapsed/2)
            bmc_half_elapsed = bmc_latency / 2.0
            mcp_half_elapsed = mcp_latency / 2.0
            corrected_bmc_sec = float(bmc_sec) + bmc_half_elapsed
            corrected_mcp_sec = mcp_sec_f + mcp_half_elapsed
            corrected_diff = corrected_bmc_sec - corrected_mcp_sec
            result[f"corrected_diff_{sample_num}_sec"] = corrected_diff

            match_ok = abs(corrected_diff) <= tolerance_sec
            result[f"time_match_{sample_num}"] = match_ok
            status = "[PASS]" if match_ok else "[FAIL]"

            self.log.info("[DEBUG TIMESTAMPS]")
            self.log.info(f"  MCP (ms since epoch) : {mcp_ms}")
            self.log.info(f"  MCP (as seconds)     : {mcp_sec_f:.3f}")
            self.log.info(f"  BMC (seconds)        : {bmc_sec}")

            self.log.info(f"  Raw diff (BMC - MCP) : {raw_diff:+.3f} s")
            self.log.info(
                "  Half elapsed used     : "
                f"BMC={bmc_half_elapsed:+.3f}s, MCP={mcp_half_elapsed:+.3f}s"
            )
            self.log.info(
                "  Corrected diff (BMC - MCP): "
                f"({bmc_sec:.3f} + {bmc_half_elapsed:+.3f}) - "
                f"({mcp_sec_f:.3f} + {mcp_half_elapsed:+.3f}) = "
                f"{corrected_diff:+.3f} s"
            )
            self.log.info(
                f"  {status} Sample {sample_num}: "
                f"|corrected_diff|={abs(corrected_diff):.3f} s "
                f"(tolerance={tolerance_sec:.1f} s)"
            )

            # Wait 40 seconds before next sample (except after last)
            if sample_num < sample_count:
                self.log.info(f"Waiting 40 seconds before sample {sample_num + 1}...")
                time.sleep(40)

        # Overall result
        all_matches = all(result[f"time_match_{i}"] for i in range(1, sample_count + 1))
        result["success"] = all_matches

        self.log.info("")
        self.log.info("=" * 60)
        self.log.info("PARALLEL CAPTURE TEST SUMMARY")
        self.log.info("=" * 60)
        for i in range(1, sample_count + 1):
            status = "PASS" if result[f"time_match_{i}"] else "FAIL"
            raw = result.get(f"raw_diff_{i}_sec")
            corr = result.get(f"corrected_diff_{i}_sec")
            raw_str = f"{raw:+.3f} s" if raw is not None else "N/A"
            corr_str = f"{corr:+.3f} s" if corr is not None else "N/A"
            self.log.info(
                f"  Sample {i}: {status}  " f"raw={raw_str}  corrected={corr_str}"
            )
        self.log.info("=" * 60)

        if result["success"]:
            self.log.info(
                "[PASS] UTC times are synchronized "
                "(all 10 parallel samples within tolerance)"
            )
        else:
            self.log.error("[FAIL] UTC time synchronization failed")
            for i in range(1, sample_count + 1):
                if not result[f"time_match_{i}"]:
                    corr = result.get(f"corrected_diff_{i}_sec")
                    self.log.error(
                        f"  - Sample {i}: |corrected_diff|="
                        f"{abs(corr):.3f} s exceeds {tolerance_sec:.1f} s"
                    )
        self.log.info("=" * 60)

        if self.bmc_cli.is_open():
            self._close_bmc_cli_if_open()
            self.log.info("BMC fast path session closed")

        return result
