# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests soc_mission_mode_disable Die_0
"""
import time
import sys
import os

from pathlib import Path
from typing import Any

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "kng_pythia_libs"))
from kng_pythia_test_setup import KngPythiaTestSetup
from RscmHelperLibrary import RscmHelperLibrary

from library.utilities.uefi_utils import UEFI_helper

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest


# Class name must match file name for Robot Framework Library usage
class soc_mission_mode_disable(EchoFallsBaseTest):
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
        name: str = "SOC_Mission_Mode_Disable",
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
        self.apns_cli = (
            self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel()
        )

        assert self.scp_cli is not None
        assert self.apns_cli is not None

        if not self.scp_cli.is_open():  # type: ignore
            self.scp_cli.open()  # type: ignore

        if not self.apns_cli.is_open():  # type: ignore
            self.apns_cli.open()  # type: ignore

        self.bmc_cli = self.dut.mb.node_0.dcscm.bmc.cli
        if not self.bmc_cli.is_open():
            self.bmc_cli.open()

    def teardown(self):
        self.dut.teardown()

        self.apns_cli.close()
        self.scp_cli.close()
        self.bmc_cli.close()

        time.sleep(30)

    def soc_mission_mode_disable(self) -> bool:
        """
        MissionMode Disable Boot Test Steps:
            1. Setup DUT and RSCM connection.
            2. Setup MUX for SCP CLI.
            3. Wait for Win boot to complete and SAC prompt to be available.
            4. Enter UEFI and turn off MissionMode.
            5. Teardown Test.
        """

        # Step
        self.setup()

        uefi_helper = UEFI_helper(dut=self.dut, log=self.log)

        cred_path = os.environ.get("SECURE_FILE_PATH")
        if cred_path is None:
            self.log.error(f"SECURE_FILE_PATH environment variable is not set")
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

        rscm_helper.rscm_soc_reset()

        # Step
        rscm_helper.set_bmc_uart_mux_scp(self.bmc_cli)

        # Step
        self.log.info("Turning off mission mode")
        res = self.disable_mission_mode(
            rscm_helper=rscm_helper,
            scp_cli=self.scp_cli,
            apns_cli=self.apns_cli,
            uefi_helper=uefi_helper,
        )
        if res is False:
            self.log.error("Turn off mission mode: failed")
            self.test_notify(
                step="Mission Mode Disable Test",
                msg="Test Fail",
                _is_error=True,
            )
            self.teardown()
            return False

        self.test_notify(
            step="Mission Mode Disable Test", msg="Test Done", _is_error=False
        )
        self.teardown()
        return True

    def disable_mission_mode(
        self, rscm_helper: Any, scp_cli: Any, apns_cli: Any, uefi_helper: Any
    ) -> bool:
        assert rscm_helper is not None
        assert scp_cli is not None
        assert apns_cli is not None
        assert uefi_helper is not None

        if not apns_cli.is_open():
            apns_cli.open()

        try:
            is_uefi_shell_up = uefi_helper.boot_uefi(
                apns_cli=apns_cli, rscm_helper=rscm_helper
            )
            if is_uefi_shell_up is True:
                uefi_helper.change_mission_mode_setting(apns_cli=apns_cli, enable=False)
            else:
                self.log.error(f"Turn off MissionMode failed: UEFI shell boot timeout")
                return False

        except Exception as e:
            self.log.error(f"Turn off MissionMode failed: Exception in UEFI: {e}")
            return False
        return True
