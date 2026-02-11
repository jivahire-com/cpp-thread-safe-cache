# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests AP WDT error handling
"""
import time
import sys, os
from pathlib import Path
from typing import Any

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from RscmHelperLibrary import RscmHelperLibrary
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
from library.utilities.bmc_utils import set_bmc_uart_mux
import re

# Class name must match file name for Robot Framework Library usage
class ap_wdt_test(EchoFallsBaseTest):

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
        name: str = "AP WDT Error Handling Test",
        number: str = "NaN",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = "KingsgateSVP",
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

        # Ensure channel is closed before reopening to avoid stale connections
        try:
            if apns_cli.is_open():
                apns_cli.close()
        except Exception:
            pass  # Ignore errors when closing

        # Wait a moment for connection to fully close
        time.sleep(2)

        # Open the channel with retry logic
        max_retries = 3
        for attempt in range(max_retries):
            try:
                apns_cli.open()
                break
            except Exception as e:
                self.log.warning(f"APNS channel open attempt {attempt + 1} failed: {e}")
                if attempt < max_retries - 1:
                    time.sleep(5)
                else:
                    self.log.error("Failed to open APNS channel after retries")
                    raise

        # Clear buffer before starting to ensure we don't match stale data
        apns_cli.clear_buffer()
 
        self.log.info("Setup boot options")
        rscm_helper.rscm_set_boot_option(option="ConfApp")
 
        self.log.info("Booting UEFI Shell...")
        return self._navigate_to_uefi_shell(apns_cli)

    def setup_mission_mode_and_reset(
        self, rscm_helper: Any, apns_cli: Any, enable: bool
    ) -> bool:
        """
        Boot to UEFI shell, change mission mode setting, and reset SOC.

        Args:
            rscm_helper: RSCM helper for SOC reset operations.
            apns_cli: APNS CLI channel for communication.
            enable: True to enable mission mode, False to disable.

        Returns:
            bool: True if successful, False otherwise.
        """
        assert rscm_helper is not None
        assert apns_cli is not None

        try:
            # Boot to UEFI shell
            is_uefi_shell_up = self.reset_and_boot_uefi(apns_cli, rscm_helper)

            if is_uefi_shell_up:
                # Change mission mode setting with correct GUID and format
                MISSION_GUID = "9b35e482-22d7-47b1-ae6f-6a634cc01f87"
                mode_value = "01" if enable else "00"

                self.log.info(f"{'Enabling' if enable else 'Disabling'} MissionMode")
                command = f"setvar MissionMode -guid {MISSION_GUID} -bs -rt -nv ={mode_value}"
                self._send_uefi_command(apns_cli, command)
                apns_cli.read_until(key="Shell>", timeout_seconds=45)

                # Reset SOC to apply UEFI settings
                rscm_helper.rscm_soc_reset()
                time.sleep(60)

            else:
                self.log.error("UEFI shell boot timeout")
                return False

        except Exception as e:
            self.log.error(f"Exception in setup_mission_mode_and_reset in UEFI: {e}")
            return False
        return True

    def _navigate_to_uefi_shell(self, apns_cli: Any) -> bool:
        """
        Navigate through boot menu to reach UEFI shell.

        Args:
            apns_cli: APNS CLI channel for communication.

        Returns:
            bool: True if UEFI shell is up, False otherwise.
        """
        try:
            self.log.info("Waiting for boot menu...")
            apns_cli.read_until(key="Available Options", timeout_seconds=1200)
            apns_cli.read_until(key="Exit this menu", timeout_seconds=30)

            self.log.info("Entered boot menu. Selecting option 2 for boot options...")
            apns_cli.write_line(write_string="2")
            apns_cli.read_until(key="Boot Options:", timeout_seconds=30)
            apns_cli.read_until(key="Select Index to boot to the corresponding option", timeout_seconds=10)

            time.sleep(2)

            self.log.info("Selecting option 3 to boot to UEFI shell...")
            apns_cli.write_line(write_string="3")

            time.sleep(1)

            # Send ESC twice to ensure we end up in shell
            self.log.info("Sending ESC keys...")
            apns_cli.write_line(write_string="\x1b\x1b")

            # Verify UEFI is booting
            apns_cli.read_until(key="seconds to skip", timeout_seconds=30)

            # Send ESC again to skip startup
            apns_cli.write_line(write_string="\x1b\x1b")

            time.sleep(5)

            # Run pci command to verify shell is interactive
            self.log.info("Verifying UEFI shell with pci command...")
            apns_cli.write_line(write_string="pci\r\n")

            time.sleep(20)

            self.log.info("UEFI shell is up and interactive")
            return True

        except TimeoutError:
            self.log.error("Timeout navigating to UEFI shell")
            return False
        except Exception as e:
            self.log.error(f"Error navigating to UEFI shell: {e}")
            return False

    def _send_uefi_command(self, apns_cli: Any, command: str) -> None:
        """
        Send a command to UEFI shell in chunks to avoid buffer issues.

        Args:
            apns_cli: APNS CLI channel for communication.
            command: Command to send.
        """
        full_command = f"{command}\r\r"
        apns_cli.clear_buffer()

        # Send command in chunks
        chunk_size = 16
        for i in range(0, len(full_command), chunk_size):
            chunk = full_command[i:i + chunk_size]
            apns_cli.write(write_string=chunk)
            time.sleep(0.1)

        if not full_command.endswith("\n"):
            apns_cli.write(write_string="\n")

    def ap_wdt_test(self) -> bool:
        """
        AP WDT error handling test:
            1. Setup the Test
            2. Perform the AP WDT error handling test with CLI commands
            3. Check for pass criteria in  SCP UART log
            4. Teardown Test.
        """
      
        self.log.info("Running AP WDT error handling test  . . .") 
        self.dut.setup()

        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)
        elif self.dut.get_dut_type() == DeviceType.RVP:
            self.log.warning("Device type is RVP. Performing SoC Reset ...")
            cred_path = os.environ.get('SECURE_FILE_PATH')
            creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
            rscm_helper = RscmHelperLibrary(rm_host=self.host_config.rack_scm.host, bmc_host=self.dut.mb.node_0.dcscm.bmc.ip, rm_user=creds['RM_USER'], rm_password=creds['RM_PASSWORD'], bmc_user=creds['BMC_USER'], bmc_password=creds['BMC_PASSWORD'], node=self.host_config.node_id)

            # Setup mission mode before running the test
            apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager
            apns_cli = apns_connection.get_current_channel()
            if not self.setup_mission_mode_and_reset(rscm_helper, apns_cli, enable=False):
                self.log.error("Failed to setup mission mode")
                self.dut.teardown()
                time.sleep(30)
                return False

            rscm_helper.rscm_soc_reset()
            set_bmc_uart_mux(self.dut, self.log, "SCP")

        scp_connection = self.dut.mb.node_0.soc.primary_die.scp.channel_manager

        # Ensure the host config file used alongside this test has these connections defined.
        assert scp_connection is not None
    
        scp_connection.get_current_channel().open()
        if not scp_connection.get_current_channel().is_open():
            self.log.error("scp_connection is not open")
            self.dut.teardown()
            time.sleep(30)
            return False

        try:
            self.log.info("Apcore Power On..")
            scp_connection.get_current_channel().read_until(key="Apcore Power On Request Completed", timeout_seconds=1800)
            time.sleep(30)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="APcore power on", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        command = "hm hm_inject_err 17 0 0 0 0 0 0 0"
        self.log.info(f"Submitting {command}\n")
        scp_connection.get_current_channel().write_line(write_string=command)
        try:
            scp_connection.get_current_channel().read_until(key="Invoke AP_WDT EINJ handler", timeout_seconds=60)
            scp_connection.get_current_channel().read_until(key="AP WDT occurred on a previous boot", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error AP WDT Error handling {e}")
            self.test_notify(step="AP WDT Error Handling", msg="Test Fail", _is_error=True)
            scp_connection.get_current_channel().close()
            self.dut.teardown()
            time.sleep(30)
            return False 

        scp_connection.get_current_channel().close()
        time.sleep(30)
            
        self.test_notify(step="AP WDT Error Handling", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True
