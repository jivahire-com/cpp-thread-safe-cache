# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests that checks for sdm pcie enumeration completion from AP core0.
"""

import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class sdm_pcie_test(EchoFallsBaseTest):

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
        name: str = None,
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
    
    def sdm_pcie_test(self, pass_logs, fail_logs):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for AP to be up. Send 'ap_bm pcie_test' command 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running SDM PCIE Test. . .")
        self.dut.setup()

        core_com_channel=self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel()

        if (self.dut.get_dut_type() == DeviceType.BIGFPGA):
            KngPythiaTestSetup.reset_fpga_load_prodfw(self)
        
        elif (self.dut.get_dut_type() == DeviceType.SVP):
            core_com_channel.open()
            core_com_channel.is_open()

        else:
            self.log.error("Unsupported DUT type")
            self.dut.teardown()
            return False

        self.log.info("Submitting ap_bm pcie_test command . . .") 
        command_response=core_com_channel.execute_command(command="ap_bm pcie_test", command_args= "")
        
        if (self.dut.get_dut_type() == DeviceType.BIGFPGA):
            test_result = KngPythiaTestIF.parse_log(self=self, log_lines=command_response[1], pass_logs=pass_logs, fail_logs=fail_logs)

        elif (self.dut.get_dut_type() == DeviceType.SVP):
            command_response=KngPythiaTestIF.read_from_uart(self=self, connection=self.dut.mb.node_0.soc.primary_die.apns.channel_manager, num_lines=35)
            test_result=KngPythiaTestIF.parse_log(self=self, log_lines=command_response, pass_logs=pass_logs, fail_logs=fail_logs)
        
        else:
            self.log.error("Unsupported DUT type")
            self.dut.teardown()
            return False

        self.dut.teardown()
        return test_result