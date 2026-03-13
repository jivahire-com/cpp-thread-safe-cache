# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests SOC DDR_ECC_CE_Error_Injection Die_0
"""
import time
import sys
import os
import glob
from pathlib import Path
from typing import Any

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "kng_pythia_libs"))
from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from RscmHelperLibrary import RscmHelperLibrary

from library.utilities.bmc_utils import run_bmc_commands

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest


# Class name must match file name for Robot Framework Library usage
class ddr_ecc_ce_error_injection_test(EchoFallsBaseTest):
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
        name: str = "SOC_DDR_ECC_CE_Error_Injection_Test",
        number: str = "NaN",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = "KingsgateRVP",
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

        self.scp_cli = (
            self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        )

        assert self.scp_cli is not None

        if not self.scp_cli.is_open():
            self.scp_cli.open()

        self.bmc_cli = self.dut.mb.node_0.dcscm.bmc.cli
        if not self.bmc_cli.is_open():
            self.bmc_cli.open()

    def teardown(self):
        self.dut.teardown()

        self.scp_cli.close()
        self.bmc_cli.close()

        time.sleep(30)

    def ddr_ecc_ce_error_injection_test(self) -> bool:
        """
        Test Steps:
            1. Setup DUT and RSCM connection.
            2. SOC reset and setup MUX for SCP CLI.
            3. Wait for Apcore Power On.
            4. Remove existing dump files and CPER logs then make a new subscription.
            5. Send DDR ECC CE Error Injection.
            6. Download CPER dump file and check its content.
            7. Teardown Test.
        """

        # Step
        self.setup()

        cred_path = os.environ.get("SECURE_FILE_PATH")
        if cred_path is None:
            self.log.error(f"SECURE_FILE_PATH environment variable is not set")
            self.test_notify(
                step="DDR ECC CE Error Injection", msg="Test Fail", _is_error=True
            )
            self.teardown()
            return False

        creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
        rscm_helper = RscmHelperLibrary(
            rm_host=self.host_config.rack_scm.host,
            bmc_host=self.dut.mb.node_0.dcscm.bmc.ip,
            rm_user=creds["RM_USER"],
            rm_password=creds["RM_PASSWORD"],
            bmc_user=creds["BMC_USER"],
            bmc_password=creds["BMC_PASSWORD"],
            node=self.host_config.node_id,
        )

        bmc_account = creds["BMC_USER"]
        bmc_password = creds["BMC_PASSWORD"]
        bmc_host = self.dut.mb.node_0.dcscm.bmc.ip
        redfish_port = "2443"
        redfish_dest_port = "8080"

        # Step
        rscm_helper.rscm_soc_reset()
        rscm_helper.set_bmc_uart_mux_scp(self.bmc_cli)

        # Step
        try:
            self.log.info("Apcore Power On..")
            self.scp_cli.read_until(
                key="Apcore Power On Request Completed", timeout_seconds=1800
            )
            time.sleep(30)
        except Exception as e:
            self.log.error(f"Error reading scp_cli UART: {e}")
            self.test_notify(
                step="DDR ECC CE Error Injection", msg="Test Fail", _is_error=True
            )
            self.teardown()
            return False

        # Step
        try:
            self.log.info(f"Removing existing dump files")
            cmds = []
            match = glob.glob("cper_dump*.dump")
            for filename in match:
                command = f"rm {filename}"
                cmds.append(command)
            res = run_bmc_commands(self.dut, self.log, cmds)
            cmds.clear()

            self.log.info(f"Removing all CPER entries")
            command = f'curl --silent -k -u {bmc_account}:{bmc_password} -H "Content-Type: application/json" -X GET https://{bmc_host}/redfish/v1/Systems/system/LogServices/Dump/Entries | jq -r \'.Members[] | "\\(.Id)"\''
            cmds.append(command)
            res = run_bmc_commands(self.dut, self.log, cmds)
            cmds.clear()

            stdout = ""
            for dict_item in res:
                stdout = dict_item.get("stdout", "")
                lines = stdout.splitlines()
                for log_id in lines:
                    if log_id.strip() != "":
                        cmds.append(
                            f"curl -sk -u {bmc_account}:{bmc_password} --silent -X DELETE https://{bmc_host}/redfish/v1/Systems/system/LogServices/Dump/Entries/{log_id}"
                        )
            res = run_bmc_commands(self.dut, self.log, cmds)
            cmds.clear()

            self.log.info(f"Subscribe new redfish event")
            command = f'curl --silent -k -u {bmc_account}:{bmc_password} -X POST https://{bmc_host}:{redfish_port}/redfish/v1/EventService/Subscriptions -d \'{{"Destination": "https://{bmc_host}:{redfish_dest_port}/", "MessageIds": ["PlatformError"], "Protocol": "Redfish"}}\''
            cmds.append(command)
            res = run_bmc_commands(self.dut, self.log, cmds)
            cmds.clear()

            # Step
            self.log.info(f"Injecting a DDR ECC CE")
            command = "hm"
            self.log.info(f"Writing command to SCP channel: {command}")
            self.scp_cli.write_line(write_string=command)
            command = "hm_inject_err 0 0 0 0 0x17 0 0 0"
            self.log.info(f"Writing command to SCP channel: {command}")
            self.scp_cli.write_line(write_string=command)
            command = ".."
            self.log.info(f"Writing command to SCP channel: {command}")
            self.scp_cli.write_line(write_string=command)
            self.log.info(f"Waiting for 30 seconds for hm_error_injection...")
            time.sleep(30)

            self.log.info(f"Fetching last entry's AdditionalDataURI")
            command = f"curl --silent -k -u {bmc_account}:{bmc_password} -H 'Content-Type: application/json' -X GET https://{bmc_host}/redfish/v1/Systems/system/LogServices/Dump/Entries | jq -r '.Members[] | \"\\(.AdditionalDataURI)\"' | tail -1"
            cmds.append(command)
            res = run_bmc_commands(self.dut, self.log, cmds)
            cmds.clear()

            stdout = ""
            for dict_item in res:
                stdout = dict_item.get("stdout", "")

            additionalURI = stdout.strip()
            if additionalURI == "":
                self.log.error(f"No CPER AdditionalURI")
                self.test_notify(
                    step="DDR ECC CE Error Injection", msg="Test Fail", _is_error=True
                )
                self.teardown()
                return False

            # Step
            self.log.info(f"Downloading CPER dump file from additionalURI")
            command = f"curl --silent -kOJ -H 'Accept:application/octet-stream' https://{bmc_account}:{bmc_password}@{bmc_host}{additionalURI}"
            cmds.append(command)
            res = run_bmc_commands(self.dut, self.log, cmds)
            cmds.clear()

            self.log.info(f"Checking the CPER dump file")
            command = f"ls | grep cper_dump"
            cmds.append(command)
            res = run_bmc_commands(self.dut, self.log, cmds)
            cmds.clear()

            stdout = ""
            for dict_item in res:
                stdout = dict_item.get("stdout", "")

            if stdout.strip() == "":
                self.log.error(f"No CPER dump file")
                self.test_notify(
                    step="DDR ECC CE Error Injection", msg="Test Fail", _is_error=True
                )
                self.teardown()
                return False
            else:
                dump_filename = stdout.strip().splitlines()[0]
                self.log.info(f"Found CPER dump file: {dump_filename}")

                # Dump CPER file content
                command = f"hexdump -C {dump_filename}"
                cmds.append(command)
                res = run_bmc_commands(self.dut, self.log, cmds)
                cmds.clear()

                stdout = ""
                for dict_item in res:
                    stdout = dict_item.get("stdout", "")
                    self.log.info(f"Dump CPER file content: \n{stdout}")

                is_valid = False
                # Check "CPER" exists
                if "43 50 45 52" in stdout:
                    # Check ACPI_ERROR_TYPE_VENDOR_DDR exists
                    if "4d 3c 2b 1a 6f 5e 9a 78  ab cd ef 01 23 45 67 89" in stdout:
                        is_valid = True
                        self.log.info(
                            f"CPER content check passed: ACPI_ERROR_TYPE_VENDOR_DDR"
                        )
                    # Check ACPI_ERROR_TYPE_PLATFORM_MEMORY exists
                    elif "14 11 bc a5 64 6f de 4e  b8 63 3e 83 ed 7c 83 b1" in stdout:
                        is_valid = True
                        self.log.info(
                            f"CPER content check passed: ACPI_ERROR_TYPE_PLATFORM_MEMORY"
                        )

                if not is_valid:
                    self.log.error(f"CPER dump file content check failed")
                    self.test_notify(
                        step="DDR ECC CE Error Injection",
                        msg="Test Fail",
                        _is_error=True,
                    )
                    self.teardown()
                    return False

        except Exception as e:
            self.log.error(f"Exception in SCP Error Injection and BMC redfish API: {e}")
            self.test_notify(
                step="DDR ECC CE Error Injection", msg="Test Fail", _is_error=True
            )
            self.teardown()
            return False

        self.test_notify(
            step="DDR ECC CE Error Injection", msg="Test Done", _is_error=False
        )
        self.teardown()
        return True
