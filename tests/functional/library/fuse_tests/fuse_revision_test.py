# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for Config Manager CLI List Command.
"""
import time
import sys, os
import subprocess
import json
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from RscmHelperLibrary import RscmHelperLibrary
from library.utilities.bmc_utils import set_bmc_uart_mux
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
import re

class fuse_revision_test(EchoFallsBaseTest):

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
        name: str = "fuse_revision_test",
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
    def fuse_revision_test(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for SCP to be up. Send 'sample_detail' command 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running Fuse Revision Test command Test. . .")
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
            hsp_channel=self.dut.mb.node_0.soc.primary_die.hsp.channel_manager.get_current_channel()
            hsp_channel.open()
            assert hsp_channel.is_open()
            
        core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_channel.open()
        assert core_com_channel.is_open()
        
        try:
            self.log.info("Waiting for Heartbeat Msg")
            core_com_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except TimeoutError as e:
            self.log.error(f"Timeout occurred while reading ScpHeartBeat: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False
        except IOError as e:
            self.log.error(f"I/O error occurred while reading ScpHeartBeat: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info("Submitting fuse revision Test command . . .")
        command="fuse"
        core_com_channel.write_line(write_string=command)

        for attempt in range(2):
            command="sample_detail"
            self.log.info(f"Submitting sample detail command {command} . . .") 
            core_com_channel.write_line(write_string=command)

            try:
                core_com_channel.read_until(key="Developer sample", timeout_seconds=300)
                self.log.info("sample detail command executed Successfully . . .")
            except Exception as e:
                self.log.error(f"Error reading SCP UART: {e}")
                core_com_channel.close()
                self.test_notify(step="fuse sample detail command ", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            
        core_com_channel.close()
        self.test_notify(step="fuse sample detail command", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)
        return True