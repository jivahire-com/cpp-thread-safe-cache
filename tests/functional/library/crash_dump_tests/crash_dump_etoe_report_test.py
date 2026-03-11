# Copyright (c) Microsoft Corporation. All rights reserved.

import os
import re
import sys
import shutil
import time
import subprocess
import json
import paramiko
import traceback
from pathlib import Path
from typing import Optional, List, Tuple

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from RscmHelperLibrary import RscmHelperLibrary
from library.utilities.bmc_utils import set_bmc_uart_mux, BmcCommandError
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class crash_dump_etoe_report_test(EchoFallsBaseTest):
  
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
    
   # Default directories and filenames
    BMC_EVENT_DIR = "/usr/share/pldm/eventdata"       # Where .b64/.cd files appear
    BMC_STAGING_DIR = "/var/wcs/home"                # Where we copy and rename
    BMC_STAGING_NAME = "crash_dump.b64"              # Renamed file on BMC

    RM_SHARE_PATH = "/usr/srvroot/shared/crash_dump.b64"  # RM shared path
    RM_PORT = 2200
    LOCAL_B64_NAME = "crash_dump.b64"

    def __init__(
        self,
        name: str = "crash_dump_test",
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
    def setup(self):
        self.dut.setup()
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning(
                "Device type is bigFPGA. Performing an additional OOB reset ..."
            )
            KngPythiaTestSetup.fpga_oob_reset(self.log)
        elif self.dut.get_dut_type() == DeviceType.RVP:
            self.log.warning("Device type is RVP. Performing SoC Reset ...")
            cred_path = os.environ.get("SECURE_FILE_PATH")
            creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
            self.rscm_helper = RscmHelperLibrary(
                rm_host=self.host_config.rack_scm.host,
                bmc_host=self.dut.mb.node_0.dcscm.bmc.ip,
                rm_user=creds["RM_USER"],
                rm_password=creds["RM_PASSWORD"],
                bmc_user=creds["BMC_USER"],
                bmc_password=creds["BMC_PASSWORD"],
                node=self.host_config.node_id,
            )
            self.rscm_helper.rscm_soc_reset()
            hsp_channel=self.dut.mb.node_0.soc.primary_die.hsp.channel_manager.get_current_channel()
            hsp_channel.open()
            assert hsp_channel.is_open()
            self.bmc_cli = self.dut.mb.node_0.dcscm.bmc.cli
            self.bmc_cli.open()
            assert self.bmc_cli.is_open()

            core_scp_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
            
            core_scp_channel.open()
            assert core_scp_channel.is_open()

            core_mcp_connection = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
            core_mcp_connection.open()
            assert core_mcp_connection.is_open()

            apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel()
            apns_connection.open()
            assert apns_connection.is_open()
           
            set_bmc_uart_mux(self.dut, self.log, "MCP")
            
            self.log.info(f"Waiting for SAC> prompt...\n")
            apns_connection.read_until(key="SAC>", timeout_seconds=1000)

    def teardown(self):
        if self.dut.get_dut_type() == DeviceType.RVP:
            self.rscm_helper.set_bmc_uart_mux_scp(self.bmc_cli)
        self.dut.teardown()
        time.sleep(10)
    
    def __bmc_execute_command(self, command):
        cmd_str = command
        result, stdout, stderr = self.bmc_cli.execute_command(
            command=cmd_str, command_args=[], timeout_secs=1
        )
        if result != 0:
            self.log.info(f"~$ failed to execute command: {cmd_str}")
        return result, stdout, stderr
    
      
    def wait_for_mcp_crashdumpevent(self, timeout_seconds: int = 420) -> str:
        """
        Wait for MCP crash dump event indication on BMC UART.

        Args:
            timeout_seconds: Maximum time to wait for the event.
        """

        self.log.info("waiting for MCP crash dump transmitting done on MCP uart...")

        def _find_b64() -> Optional[str]:
            cmd = f"find {self.BMC_EVENT_DIR} -maxdepth 1 -type f -name '*.b64' -print | head -n 1"
            rc, out, _ = self.__bmc_execute_command(cmd)
            return out.strip() if rc == 0 and out.strip() else None
        
        def _mtime(path: str) -> Optional[int]:
            rc, out, _ = self.__bmc_execute_command(f"stat -c %Y {path} 2>/dev/null || busybox stat -c %Y {path} 2>/dev/null")
            return int(out.strip()) if rc == 0 and out.strip().isdigit() else None
        
        def _remote_now() -> Optional[int]:
            rc, out, _ = self.__bmc_execute_command(f"date +%s")
            return int(out.strip()) if rc == 0 and out.strip().isdigit() else None
        
        def _is_stable(path: str) -> bool:
            stable_age_secs =2
            if stable_age_secs <= 0:
                return True
            mtime = _mtime(path)
            now = _remote_now()
            return mtime is not None and now is not None and (now - mtime) >= stable_age_secs
        
        def _copy_and_rename(src: str) -> str:
            dst = f"{self.BMC_STAGING_DIR}/{self.BMC_STAGING_NAME}"
            rc, _, err = self.__bmc_execute_command(f"cp -- '{src}' '{dst}'")
            if rc != 0:
                raise RuntimeError(f"Failed to copy {src} -> {dst}: {err}")
            return dst
            
        mcp_channel = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
        mcp_channel.read_until(key="PLDM crash dump transfer completed", timeout_seconds=timeout_seconds)
        time.sleep(40)
        candidate = _find_b64()
        if candidate and _is_stable(candidate):
            dest_path = _copy_and_rename(candidate)
            self.log.info(f"Copied and renamed {candidate} to {dest_path}")
            return dest_path
        
        
    def make_crash_dump_command(self):
        command = "crashdump trig_except"
        key = "crash_dump_wait_forever"
        timeout_seconds = 100

        max_retries = 3            # number of *additional* retries after the first attempt
        backoff_seconds = 3        # wait before re-issuing command on timeout

        self.log.info(f"Submitting {command}\n")

        core_mcp_channel = self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
        if not getattr(core_mcp_channel, "IsOpen", False):
           try:
              core_mcp_channel.open()
           except Exception as e:
              self.log.error(f"Failed to open MCP channel: {e}")
              self.test_notify(step="Crash Dump.", msg="Channel Open Failed", _is_error=True)
              return False

        # Utility to check if an exception represents a read timeout.
        def _is_timeout_error(exc: Exception) -> bool:
           # Common patterns: TimeoutError or message contains 'timeout'
           if isinstance(exc, TimeoutError):
              return True
           msg = str(exc).lower()
           return "timeout" in msg or "timed out" in msg

        attempts = 0
        last_exc = None

        while attempts <= max_retries:
           try:
              # Issue command *each attempt* as requested
              self.log.info(f"[CrashDump Attempt {attempts+1}/{max_retries+1}] write_line: {command}")
              core_mcp_channel.write_line(write_string=command)

              # Wait for the key
              crashdump_output = core_mcp_channel.read_until(key=key, timeout_seconds=timeout_seconds)
              self.log.info(f"Crash Dump Output (attempt {attempts+1}):\n{crashdump_output}")

              # Keep your stabilization pause after success
              time.sleep(50)
              return True

           except Exception as e:
              last_exc = e
              # Only retry if timeout — otherwise fail fast
              if _is_timeout_error(e):
                  attempts += 1
                  if attempts <= max_retries:
                      self.log.warning(
                          f"read_until timeout on attempt {attempts}/{max_retries+1}. "
                          f"Retrying after {backoff_seconds}s..."
                      )
                      time.sleep(backoff_seconds)
                      continue
                  else:
                      self.log.error(
                        f"read_until timed out after {max_retries+1} attempts. "
                        f"Finalizing with failure."
                      )
              else:
                  # Non-timeout exception, fail immediately
                  self.log.error(
                    "Non-timeout error during crash dump:\n"
                    + "".join(traceback.format_exception_only(type(e), e))
                  )

               # Cleanup and notify on final failure
              try:
                  core_mcp_channel.close()
              except Exception as close_exc:
                  self.log.warning(f"Error while closing MCP channel: {close_exc}")

              self.test_notify(step="Crash Dump.", msg="Test Fail", _is_error=True)
              time.sleep(30)
              return False

        # Shouldn’t reach here, but in case:
        self.log.error("Unexpected fall-through in make_crash_dump_command.")
        return False

    
    def remove_event_files(self, pattern: str = "/usr/share/pldm/eventdata/*.b64") -> bool:
        """
        Remove all .b64 and .cd files from the specified directory on the BMC.

        Args:
            directory: Directory to clean up (default: /usr/share/pldm/eventdata).

        Returns:
            True if command executed successfully, False otherwise.
        """
        # Use find to match both patterns and delete safely
        
        cmd = f"rm -f {pattern}"

        rc, out, err = self.__bmc_execute_command(cmd)

        if rc == 0:
           self.log.info(f"Removed files matching: {pattern}")
           # Optional: verify remaining count (does not affect return value)
           verify_cmd = f"ls -1 {pattern} 2>/dev/null | wc -l"
           vrc, vout, _ = self.__bmc_execute_command(verify_cmd)
           try:
               remaining = int(vout.strip()) if (vrc == 0 and vout and vout.strip().isdigit()) else None
           except Exception:
               remaining = None
           if remaining is not None:
               self.log.info(f"Remaining matches after cleanup: {remaining}")
               return True
           else:
               self.log.error(f"Failed to remove files matching {pattern}: rc={rc}, stderr={err}")


    def copy_BMC_to_RSCM(self)-> bool:
        """
        Download a file from the BMC host to a local OS path using SSH tunnel.
        """
        command = f"set system file get -i {self.host_config.node_id} -f crash_dump.b64 -d /var/wcs/home"
        stdout, stderr = self.rscm_helper.execute_rm_command(command)
        if stderr:
            print(f"Error download_bmc_file to /var/wcs/home: {stderr}")
            raise AssertionError
        else:
            print(f"download_bmc_file to /var/wcs/home successfully.")
            return True
        time.sleep(3)
    
    def copy_rm_to_local(self) -> bool:
        """
       Copy crash_dump.b64 from Rack Manager to local  $REPO_APP_ROOT folder using scp.

       Source: /usr/srvroot/shared/crash_dump.b64 on RM
       Destination: $REPO_APP_ROOT/crash_dump.b64

        Returns:
        True if copy succeeds, False otherwise.
        """
        # Resolve destination path
        repo_root = os.environ.get("REPO_APP_ROOT")
        if not repo_root:
           raise EnvironmentError("REPO_APP_ROOT environment variable is not set.")
        cred_path = os.environ.get("SECURE_FILE_PATH")
        creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
        dest_dir = os.path.join(repo_root)
        os.makedirs(dest_dir, exist_ok=True)
        dest_path = os.path.join(dest_dir, "crash_dump.b64")

        # RM connection details (replace with actual values or class attributes)
        rm_host = self.host_config.rack_scm.host      # e.g., "10.127.114.140"
        rm_user = "root"                              # or creds["RM_USER"]
        rm_port = 22                                  # custom SSH port if needed
        src_path = "/usr/srvroot/shared/crash_dump.b64"
        self.log.info(f"Copying crash_dump.b64 from RM ({rm_host}:{rm_port}) to local {dest_path}...")
         
        transport = None
        sftp = None
        try:
           transport = paramiko.Transport((rm_host, rm_port))
           transport.connect(username=rm_user, password=creds["RM_PASSWORD"])
           sftp = paramiko.SFTPClient.from_transport(transport) 
           
            # --- wait for remote file to be present and stable ---
           timeout_secs = 120
           poll_secs = 1.0
           min_bytes = 8192

           start = time.time()
           last_size = -1
           stable_for = 0.0

           while True:
                try:
                    st = sftp.stat(src_path)
                    size = st.st_size
                    # size progression / stability tracking
                    if size == last_size:
                        stable_for += poll_secs
                    else:
                        stable_for = 0.0
                        last_size = size

                    self.log.info(f"RM file size now: {size} bytes (stable_for={stable_for}s)")

                    if size >= min_bytes and stable_for >= 3.0:
                        # file seems complete enough
                        break
                except (FileNotFoundError, IOError, OSError) as e:
                    self.log.info(f"RM share file not found yet: {src_path}")

                if (time.time() - start) > timeout_secs:
                    self.log.error(f"Timeout waiting for RM file {src_path} to become stable/size>={min_bytes}")
                    return False
                time.sleep(poll_secs)
                self.log.info(
               f"Remote file seems stable, starting SFTP download "
               f"{src_path} -> {dest_path}"
              )
            # --- safe to download ---
                sftp.get(src_path, dest_path)

        except Exception as e:
            self.log.error(f"Failed to copy crash_dump.b64 from RM ({rm_host}:{rm_port}): {e}")
            return False
        finally:
            try:
                if sftp is not None:
                    sftp.close()
            finally:
                if transport is not None:
                    transport.close()

        self.log.info(f"Copied crash_dump.b64 from RM to {dest_path}")
        return True
    

    def run_crash_dump_etoe_report_test(self) -> bool:
        """
        End-to-end crash dump validation:
          1. Remove .cd/.b64 on BMC
          2. Trigger crash dump
          3. Wait for .b64 and stage to /var/wcs/home/crash_dump.b64
          4. RM pulls file (set system file get) → /usr/srvroot/shared/crash_dump.b64
          5. Copy RM → local $REPO_APP_ROOT/crash_dump.b64
          6. Run base_cd_analyze and validate fields
       
        """
        # Step 1
        self.log.info("Step 1: Cleaning BMC eventdata directory...")
        if not self.remove_event_files():
            self.log.error("Failed to clean BMC directory.")
            return False

        # Step 2
        self.log.info("Step 2: Triggering crash dump...")
        if not self.make_crash_dump_command():
            self.log.error("Crash dump trigger failed.")
            return False

        # Step 3
        self.log.info("Step 3: Waiting for .b64 and staging to /var/wcs/home...")
        try:
            staged_path = self.wait_for_mcp_crashdumpevent(timeout_seconds=600)
            self.log.info(f"Staged file at: {staged_path}")
        except Exception as e:
            self.log.error(f"Staging .b64 failed: {e}")
            return False

        # Step 4
        self.log.info("Step 4: RM pulling staged crash_dump.b64 from BMC...")
        if not self.copy_BMC_to_RSCM():
            self.log.error("Failed to copy crash dump from BMC to RM.")
            return False

        # Step 5
        self.log.info("Step 5: Copying crash_dump.b64 from RM to local...")
        if not self.copy_rm_to_local():
            self.log.error("Failed to copy crash dump from RM.")
            return False

    
        # Resolve paths
        repo_root = os.environ.get("REPO_APP_ROOT")
        if not repo_root:
          raise EnvironmentError("REPO_APP_ROOT environment variable is not set.")
        crashdump_dir = os.path.join(repo_root)
        b64_path = os.path.join(crashdump_dir, "crash_dump.b64")
        
        # Validate source file
        if not os.path.isfile(b64_path):
          raise FileNotFoundError(f"Source file not found: {b64_path}")

        # Log size for verification
        size_bytes = Path(b64_path).stat().st_size
        print(f"{b64_path} (size: {size_bytes} bytes)")
    
        file_path = Path(b64_path)
        size_bytes = file_path.stat().st_size
        json_path = os.path.join(crashdump_dir, "crash_dump.json")
        cmd_path = shutil.which("echo_falls_crash_dump_analysis_tool")
        self.log.info(f"Crash dump analysis tool path: {cmd_path}")

        if not file_path.is_file():
           raise FileNotFoundError(f"File not found: {b64_path}")

        with file_path.open("r", encoding="utf-8") as f:
           content = f.read()
        self.log.info(f"data={content[:100]}... (truncated)")
        self.log.info(f"Local crash_dump.b64 path: {b64_path} size: {size_bytes} bytes")
        self.log.info(f"Output JSON path: {json_path}")
        # Step 5: Analyze crash dump using base_cd_analyze
        self.log.info("Step 5: Running echo_falls_crash_dump_analysis_tool...")
        analyze_cmd = [
                    cmd_path,
                    "-c", b64_path,
                    "-o", json_path,
                    "--decompress"
                ]
         # Log the command
        self.log.info("cmd: " + ' '.join(analyze_cmd))
        try:
           subprocess.run(analyze_cmd, capture_output=True, text=True, check=False)
           self.log.info(f"Crash dump analyzed. JSON at {json_path}")
        except subprocess.CalledProcessError as e:
           self.log.error(f"echo_falls_crash_dump_analysis_tool failed: {e.stderr}")
           return False

        # Load JSON
        try:
           with open(json_path, "r") as f:
               data = json.load(f)
        except Exception as e:
            self.log.error(f"Failed to read JSON: {e}")
            return False

        # Extract fields
        product_id = data.get("Product_Info", {}).get("Product_Id")
        crash_core_id = data.get("Crash_Info", {}).get("Crash_Core_Id")
        crash_error_code = data.get("Crash_Info", {}).get("Crash_Error_Code")
        core_count = data.get("Product_Info",{}).get("Core_Count")

        self.log.info(f"Product_Id={product_id}, Crash_Core_Id={crash_core_id}, Core_Count={core_count},Crash_Error_Code={crash_error_code}")

        # Step 6: Validate conditions
        fail_flag = False
        if product_id != "0x92":
           self.log.error(f"Unexpected Product_Id: {product_id}")
           fail_flag = True

        if core_count != 10:
           self.log.error(f"Unexpected core_count: {core_count}")
           fail_flag = True

        if fail_flag:
           self.test_notify(step="Crash Dump Validation", msg="Test Fail", _is_error=True)
           return False

        self.log.info("Crash dump validation passed.")
        return True
