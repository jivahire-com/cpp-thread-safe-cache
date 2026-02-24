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

        scp_connection=self.dut.mb.node_0.soc.primary_die.scp.channel_manager
        apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager
        hsp_connection = self.dut.mb.node_0.soc.primary_die.hsp.channel_manager

        # Ensure the host config file used alongside this test has these connections defined.
        assert scp_connection is not None
        assert apns_connection is not None
        assert hsp_connection is not None

        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)
        elif self.dut.get_dut_type() == DeviceType.RVP:
            self.log.warning("Device type is RVP. Performing SoC Reset ...")
            cred_path = os.environ.get('SECURE_FILE_PATH')
            creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
            rscm_helper = RscmHelperLibrary(rm_host=self.host_config.rack_scm.host, bmc_host=self.dut.mb.node_0.dcscm.bmc.ip, rm_user=creds['RM_USER'], rm_password=creds['RM_PASSWORD'], bmc_user=creds['BMC_USER'], bmc_password=creds['BMC_PASSWORD'], node=self.host_config.node_id)
            rscm_helper.rscm_soc_reset()
            set_bmc_uart_mux(self.dut, self.log, "SCP")

        # Open SCP connection for reading UART
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
