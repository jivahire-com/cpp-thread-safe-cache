# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test to verify the warm start CLI commands.
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from RscmHelperLibrary import RscmHelperLibrary
from library.utilities.bmc_utils import set_bmc_uart_mux
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
import re

class warm_start_test(EchoFallsBaseTest):

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
        name: str = "Warm start CLI test",
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
    
    def warm_start_test(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for AP to be up
            3. Send Wswr command, followed by wsrd command
            4. Read response and check if data written is read back
            5. Submit wsreset CLI cmd and wait for SCP to successfully come up after a warm reset
            6. Teardown Test. 
        """
        self.log.info("Running Warm start CLI command Test. . .")
        self.dut.setup()
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
            
        core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_channel.open()
        assert core_com_channel.is_open()
        hsp_connection = self.dut.mb.node_0.soc.primary_die.hsp.channel_manager.get_current_channel()
        hsp_connection.open()
        assert hsp_connection.is_open()
        apns_connection = self.dut.mb.node_0.soc.primary_die.apns.channel_manager.get_current_channel()
        apns_connection.open()
        assert apns_connection.is_open()
        
        try:
            self.log.info("Waiting for SCP-AP Core Power ON Msg")
            core_com_channel.read_until(key="Primary AP core power on", timeout_seconds=1200)
            apns_connection.read_until(key="SAC>", timeout_seconds=1200)
            command="warm_start wsreset subsys"
            core_com_channel.write_line(write_string=command)
            core_com_channel.read_until(key="[SoS] Reset complete notification sent successfully", timeout_seconds=500)
            self.log.info("Mailbox cmd sent to HSP requesting MSCP subsys reset.. ")
            self.log.info("Waiting until SCP is released out of reset.. ")
            core_com_channel.read_until(key="Hello World - SCP!", timeout_seconds=500)
            self.log.info("SCP released out of reset")
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="Warm Reset Test", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False
        
        core_com_channel.close()
        hsp_connection.close()
        apns_connection.close()
        self.test_notify(step="Warm start CLI tests done", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True