# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests SOC Cold boot DualDie
"""
import time
import sys, os
import subprocess
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))
from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from RscmHelperLibrary import RscmHelperLibrary

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
from library.utilities.bmc_utils import set_bmc_uart_mux

# Class name must match file name for Robot Framework Library usage
class cold_start_test(EchoFallsBaseTest):

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
        name: str = "SOC_Windows_Cold_Boot_Dual_Die",
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

    def cold_start_test(self) -> bool:
        """
        SOC Cold boot test:
            1. Setup the Test and Cold Boot repeat_times.
            2. Perform the SOC reset.
            3. Look for Windows boot SAC and restart.
            4. Go to 3 until reaching repeat_times.
            5. Teardown Test.
        """

        repeat_times = 2
        
        self.dut.setup()

        cred_path = os.environ.get('SECURE_FILE_PATH')
        creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
        rscm_helper = RscmHelperLibrary(rm_host=self.host_config.rack_scm.host, bmc_host=self.dut.mb.node_0.dcscm.bmc.ip, rm_user=creds['RM_USER'], rm_password=creds['RM_PASSWORD'], bmc_user=creds['BMC_USER'], bmc_password=creds['BMC_PASSWORD'], node=self.host_config.node_id)
        rscm_helper.rscm_soc_reset()

        scp_connection = self.dut.mb.node_0.soc.primary_die.scp.channel_manager
        apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager
        set_bmc_uart_mux(self.dut, self.log, "SCP")

        # Ensure the host config file used alongside this test has these connections defined.
        assert scp_connection is not None
        assert apns_connection is not None

        for i in range(repeat_times):
            self.log.info("Running SOC Windows Dual Die cold boot test : repeat_times = %d" %(i))
        
            scp_connection.get_current_channel().open()
            if not scp_connection.get_current_channel().is_open():
                self.log.error("scp_connection is not open")
                self.dut.teardown()
                time.sleep(30)
                return False
            
            apns_connection.get_current_channel().open()
            # If connection does not open then SVP didn't launch or FPGA system has a conflict booking. So teardown and return fail
            if not apns_connection.get_current_channel().is_open():
                self.log.error("apns_connection is not open")
                self.dut.teardown()
                time.sleep(30)
                return False

            self.log.info("Reading APNS UART for SAC")
            try:
                scp_connection.get_current_channel().read_until(key="Primary AP core power on", timeout_seconds=1200)
                apns_connection.get_current_channel().read_until(key="SAC>", timeout_seconds=1200)
                apns_connection.get_current_channel().read_until(key="CMD command is now available", timeout_seconds=250)
                command = "restart"
                self.log.info("Writing command to APNS channel: %s" % command)
                apns_connection.get_current_channel().write_line(write_string=command)
                time.sleep(60) # Delay for next cold reset
            except Exception as e:
                self.log.error(f"Error reading APNS UART: {e}")
                apns_connection.get_current_channel().close()
                scp_connection.get_current_channel().close()
                self.test_notify(step="Windows Boot Shell", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False

            scp_connection.get_current_channel().close()
            apns_connection.get_current_channel().close()
            time.sleep(30)
            
        self.test_notify(step="Windows Boot Shell", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True
