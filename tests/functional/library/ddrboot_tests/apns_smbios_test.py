# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests anps ambios test
"""
import time
import sys, os
import re
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))
from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from BmcHelperLibrary import BmcHelperLibrary

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class apns_smbios_test(EchoFallsBaseTest):

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
        name: str = "apns_smbios_Die0_Test",
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
    
    def apns_smbios_test(self):
        """
        APNS smbios test :
            1. Setup the Test.
            2. Look for the SCP ddrboot.
            3. Teardown Test.
        """
        self.log.info("Running APNS smbios test . . .")
        apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager
        assert apns_connection is not None
        self.dut.setup()
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)
            
        apns_connection.get_current_channel().open()
        # If connection does not open then SVP didn't launch or FPGA system has a conflict booking. So teardown and return fail
        if not apns_connection.get_current_channel().is_open():
            self.dut.teardown()
            time.sleep(30)
            return False
        try:
            self.log.info("Waiting for UEFI shell prompt...")
            apns_connection.get_current_channel().read_until(key="UEFI Interactive Shell", timeout_seconds=1500)
            time.sleep(120)
            # Send SMBIOS Type 16 command
            self.log.info("Querying SMBIOS Table 16...")
            apns_connection.get_current_channel().write_line(write_string=" SMBIOSVIEW  -t 16")
            smbios16_output = apns_connection.get_current_channel().read_until(key="ExtendedMaximumCapacity", timeout_seconds=60)
            self.log.info("SMBIOS Type 16 Output:\n" + smbios16_output)
            # Validate the output
            if not smbios16_output.strip() or ("QueryType   = 16" not in smbios16_output and "Type: Physical Memory Array" not in smbios16_output):
                self.log.error("SMBIOS Type 16 output is empty or missing 'type'")
                apns_connection.get_current_channel().close()
                self.test_notify(step="SMBIOS Query", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            # Send SMBIOS Type 17 command
            self.log.info("Querying SMBIOS Table 17...")
            apns_connection.get_current_channel().write_line(write_string="SMBIOSVIEW  -t 17")
            smbios17_output = apns_connection.get_current_channel().read_until(key="Handle: 17", timeout_seconds=90)
            self.log.info("SMBIOS Type 17 Output:\n" + smbios17_output)
            if not smbios17_output.strip() or ("QueryType" not in smbios17_output and "Memory Device" not in smbios17_output):
               self.log.error("SMBIOS Type 17 output is empty or missing expected markers")
               self.test_notify(step="SMBIOS Type 17", msg="Test Fail", _is_error=True)
               apns_connection.get_current_channel().close()
               self.dut.teardown()
               time.sleep(30)
               return False
            # Each Dimn will have a Handle no in Simbios View info and the no will be 12 to 17 so I will check Handle 12 to Handle 17 all exists in simbios17_output
            required_types = list(range(12, 17))
            missing_types = [t for t in required_types if not re.search(rf"\bHandle:\s*{t}\b", smbios17_output)]

            if missing_types:
                self.log.error(f"Missing required 'Type' entries in SMBIOS Type 17 output: {missing_types}")
                self.test_notify(step="SMBIOS Types", msg=f"Missing types: {missing_types}", _is_error=True)
                apns_connection.get_current_channel().close()
                self.dut.teardown()
                time.sleep(30)
                return False
            self.test_notify(step="SMBIOS Types", msg="Test successful", _is_error=False)
            apns_connection.get_current_channel().close()
            self.dut.teardown()
            time.sleep(30)
            return True
        except Exception as e:
            self.log.error(f"Error querying SMBIOS over AP UART: {e}")
            apns_connection.get_current_channel().close()
            self.test_notify(step="SMBIOS Query", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False
       
        
