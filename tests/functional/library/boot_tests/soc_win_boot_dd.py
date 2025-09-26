# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests SOC Windows boot DualDie
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

# Class name must match file name for Robot Framework Library usage
class soc_win_boot_dd(EchoFallsBaseTest):

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
        name: str = "SOC_Windows_Boot_Dual_Die",
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
    
    def soc_win_boot_dd(self):
        """
        SOC Windows boot test:
            1. Setup the Test.
            2. Look for Windows boot SAC.
            3. Teardown Test.
        """
        self.log.info("Running SOC Windows Dual Die boot test")

        # Try importing PyYAML, install if missing
        try:
            import yaml
        except ImportError:
            print("PyYAML not found. Installing...")
            subprocess.check_call([sys.executable, "-m", "pip", "install", "pyyaml"])
            import yaml  # Try again after install

        scp_connection=self.dut.mb.node_0.soc.primary_die.scp.channel_manager
        apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager

        # Ensure the host config file used alongside this test has these connections defined.
        assert scp_connection is not None
        assert apns_connection is not None

        self.dut.setup()
        self.log.warning("Device type is SOC. Performing SOC reset ...")
        cred_path = os.environ.get('SECURE_FILE_PATH')
        creds = self.load_credentials_from_yaml(cred_path)

        rscm_helper = RscmHelperLibrary(rm_host="172.29.89.33", bmc_host="172.17.0.97", rm_user=creds['RM_USER'], rm_password=creds['RM_PASSWORD'], bmc_user=creds['BMC_USER'], bmc_password=creds['BMC_PASSWORD'])  # Fill in real host if available
        rscm_helper.rscm_soc_reset()
        
        scp_connection.get_current_channel().open()
        if not scp_connection.get_current_channel().is_open():
            self.dut.teardown()
            time.sleep(30)
            return False  
        apns_connection.get_current_channel().open()
        # If connection does not open then SVP didn't launch or FPGA system has a conflict booking. So teardown and return fail
        if not apns_connection.get_current_channel().is_open():
            self.dut.teardown()
            time.sleep(30)
            return False

        rscm_helper.rscm_set_profile("General")

        self.log.info("Reading APNS UART for SAC")

        try:
            scp_connection.get_current_channel().read_until(key="Primary AP core power on", timeout_seconds=1200)
            apns_connection.get_current_channel().read_until(key="SAC>", timeout_seconds=1200)
            apns_connection.get_current_channel().read_until(key="CMD command is now available", timeout_seconds=250)
        except Exception as e:
            self.log.error(f"Error reading APNS UART: {e}")
            apns_connection.get_current_channel().close()
            scp_connection.get_current_channel().close()
            self.test_notify(step="Windows Boot Shell", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        apns_connection.get_current_channel().close()
        scp_connection.get_current_channel().close()
        self.test_notify(step="Windows Boot Shell", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True
    
    @staticmethod
    def load_credentials_from_yaml(path):
        import yaml
        with open(path, 'r') as f:
            data = yaml.safe_load(f)
        return data

