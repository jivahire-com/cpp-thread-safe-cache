# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests SMMU UE (Uncorrectable Error) Error Injection on SCP.
"""
import time
import sys
import os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from RscmHelperLibrary import RscmHelperLibrary
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
from library.utilities.bmc_utils import run_bmc_commands, ensure_bmc_cli_open, set_bmc_uart_mux


# Class name must match file name for Robot Framework Library usage
class smmu_ue_error_injection(EchoFallsBaseTest):
    """
    SMMU UE (Uncorrectable Error) Error Injection Test - SOC specific test.

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
        name: str = "SMMU_UE_Error_Injection_Test",
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

    def _bmc_execute_command(self, command: str, raise_on_error: bool = False):
        """
        Execute a command on the BMC and return the result.
        """
        bmc_cli = ensure_bmc_cli_open(self.dut)
        ret, stdout, stderr = bmc_cli.execute_command(command=command, command_args=[])
        if stderr and raise_on_error:
            self.log.error(f"BMC command failed: {command} | Error: {stderr}")
        return ret, stdout, stderr

    def smmu_ue_error_injection(self) -> bool:
        """
        SMMU UE Error Injection test:
            1. Setup the Test.
            2. Open SCP, MCP, and APNS channels.
            3. Wait for PLDM platform event ready callback on MCP.
            4. Wait for SAC CMD to be available.
            5. Remove existing CPER dump files from BMC.
            6. Inject TCU RPSS 0 TMU HTTU RAM UE error via ap_mem_write command.
            7. Wait for Bug Check to confirm UE error was triggered.
            8. Wait 60 seconds and verify CPER dump file was created on BMC.
            9. Teardown Test.
        """
        self.log.info("Running SMMU UE Error Injection test . . .")

        self.dut.setup()

        scp_channel = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        apns_channel = self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel()

        # Ensure the host config file used alongside this test has SCP, MCP, and APNS connections defined
        assert scp_channel is not None
        assert apns_channel is not None

        # Open SCP channel
        scp_channel.open()

        if not scp_channel.is_open():
            self.log.error("Failed to open SCP channel")
            self.test_notify(step="Open_SCP_Channel", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        # Open APNS channel
        apns_channel.open()

        if not apns_channel.is_open():
            self.log.error("Failed to open APNS channel")
            scp_channel.close()
            self.test_notify(step="Open_APNS_Channel", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        # Perform SOC reset
        self.log.warning("Device type is SOC. Performing SOC reset ...")
        cred_path = os.environ.get('SECURE_FILE_PATH')
        creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
        rscm_helper = RscmHelperLibrary(rm_host=self.host_config.rack_scm.host, bmc_host=self.dut.mb.node_0.dcscm.bmc.ip, rm_user=creds['RM_USER'], rm_password=creds['RM_PASSWORD'], bmc_user=creds['BMC_USER'], bmc_password=creds['BMC_PASSWORD'], node=self.host_config.node_id)
        rscm_helper.rscm_soc_reset()

        #self.log.info("Executing i2ctransfer command on BMC...")
        #self._bmc_execute_command("i2ctransfer -f -y 3 w2@0x09 0x61 0x80")

        #self.log.info("Restarting C4143-MCTP-over-UART service on BMC...")
        #self._bmc_execute_command("systemctl restart C4143-MCTP-over-UART@ttyS3.service")

        self.log.info("Executing mctptool list on BMC...")
        pldm_found = False
        for mctp_attempt in range(1, 4):
            ret, stdout, stderr = self._bmc_execute_command("mctptool list")
            self.log.info(f"mctptool list output (attempt {mctp_attempt}/3): {stdout}")
            if "PLDM" in (stdout or ""):
                pldm_found = True
                break
            self.log.warning(f"PLDM not found in mctptool list output (attempt {mctp_attempt}/3)")
            time.sleep(2)

        if not pldm_found:
            self.log.error("PLDM not found in mctptool list output after 3 attempts")
            scp_channel.close()
            apns_channel.close()
            self.test_notify(step="PLDM_MCTP_Check", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info("PLDM found in mctptool list output")

        set_bmc_uart_mux(self.dut, self.log, "SCP")

        # Wait for SAC CMD to be available
        self.log.info("Waiting for CMD to be available . . .")
        try:
            scp_channel.read_until(key="Primary AP core power on", timeout_seconds=1200)
            apns_channel.read_until(key="SAC>", timeout_seconds=1200)
        except Exception as e:
            self.log.error(f"Error waiting for SAC CMD: {e}")
            scp_channel.close()
            apns_channel.close()
            self.test_notify(step="SAC_CMD", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        # Remove existing CPER dump files from BMC before UE injection
        self.log.info("Removing existing CPER dump files from BMC . . .")
        try:
            self._bmc_execute_command("rm -f /var/log/dump/* 2>/dev/null || true")
        except Exception as e:
            self.log.warning(f"Failed to remove existing dump files: {e}")

        # Inject TCU RPSS 0 TMU HTTU RAM UE error
        self.log.info("Injecting TCU RPSS 0 TMU HTTU RAM UE error . . .")
        command = "mem ap_mem_write 0x5008060040 0x60000502"
        self.log.info(f"Submitting command: {command}")

        try:
            scp_channel.write_line(write_string=command)
            scp_channel.read_until(key="Bug Check", timeout_seconds=120)
        except Exception as e:
            self.log.error(f"Error in injecting smmu ue error: {e}")
            scp_channel.close()
            apns_channel.close()
            self.test_notify(step="UE_Injection", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        # Verify CPER dump file was created on BMC
        self.log.info("Verifying CPER dump file was created on BMC . . .")
        time.sleep(30)
        ret, stdout, stderr = self._bmc_execute_command("ls /var/log/dump/cper*.dump 2>/dev/null")
        if ret != 0 or not stdout or "cper" not in stdout.lower():
            self.log.error("CPER dump file was not created on BMC")
            scp_channel.close()
            apns_channel.close()
            self.test_notify(step="CPER_Dump_Verification", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info(f"CPER dump file found: {stdout}")

        # Test completed successfully
        self.log.info("SMMU UE Error Injection test completed successfully")
        scp_channel.close()
        apns_channel.close()
        self.test_notify(step="SMMU_UE_Error_Injection", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True