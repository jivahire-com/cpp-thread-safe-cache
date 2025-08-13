# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests SOC UEFI boot
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))
from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from BmcHelperLibrary import BmcHelperLibrary

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class soc_uefi_boot(EchoFallsBaseTest):

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
        name: str = "SOC_UEFI_Boot_Single_Die",
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
    
    def soc_uefi_boot(self):
        """
        SOC UEFI boot test:
            1. Setup the Test.
            2. Look for UEFI Interactive Shell.
            3. Teardown Test.
        """
        self.log.info("Running SOC UEFI boot test")

        scp_connection=self.dut.mb.node_0.soc.primary_die.scp.channel_manager
        apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager

        # Ensure the host config file used alongside this test has these connections defined.
        assert scp_connection is not None
        assert apns_connection is not None

        self.dut.setup()
        self.log.warning("Device type is SOC. Performing SOC reset ...")
        bmc_helper = BmcHelperLibrary(hostname="M1120-SCM-392.svceng.com")  # Fill in real host if available
        bmc_helper.soc_reset()

        scp_connection.get_current_channel().open()            
        apns_connection.get_current_channel().open()
        # If connection does not open then SVP didn't launch or FPGA system has a conflict booking. So teardown and return fail
        if not apns_connection.get_current_channel().is_open():
            self.dut.teardown()
            time.sleep(30)
            return False

        bmc_helper.set_profile("General")
        bmc_helper.set_boot_option("ConfApp")

        self.log.info("Reading APNS UART for UEFI Interactive Shell")

        try:
            scp_connection.get_current_channel().read_until(key="Primary AP core power on", timeout_seconds=1200)
            apns_connection.get_current_channel().read_until(key="Exit this menu", timeout_seconds=500)
            command = "2"
            apns_connection.get_current_channel().write_line(write_string=command)
            apns_connection.get_current_channel().read_until(key="Select Index to boot to the corresponding option", timeout_seconds=500)
            command = "3"
            apns_connection.get_current_channel().write_line(write_string=command)
            apns_connection.get_current_channel().read_until(key="UEFI Interactive Shell", timeout_seconds=500)
        except Exception as e:
            self.log.error(f"Error reading APNS UART: {e}")
            apns_connection.get_current_channel().close()
            self.test_notify(step="UEFI_Interfactive_Shell", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        apns_connection.get_current_channel().close()
        self.test_notify(step="UEFI_Interactive_Shell", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True
