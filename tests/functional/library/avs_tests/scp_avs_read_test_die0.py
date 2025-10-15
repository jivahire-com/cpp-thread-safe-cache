# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests AVS reads Die0.
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

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
import re

# Class name must match file name for Robot Framework Library usage
class scp_avs_read_test_die0(EchoFallsBaseTest):

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
        name: str = "AVS_Read_Die0",
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
    
    def avs_read_test(self):
        """
        AVS read test:
            1. Setup the Test.
            2. Executes AVS reads.
            3. Teardown Test.
        """
        self.log.info("Running AVS read tests on Die0  . . .")
        # Try importing PyYAML, install if missing
        #try:
        #    import yaml
        #except ImportError:
        #    print("PyYAML not found. Installing...")
        #    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyyaml"])
        #    import yaml  # Try again after install

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
      
        core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_channel.open()
        assert core_com_channel.is_open()

        try:
            self.log.info(f"Reading from self.dut.mb.node_0.soc.primary_die.scp.channel_manager\n")
            core_com_channel.read_until(key="HeartBeat", timeout_seconds=1800)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="HeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        connection = self.dut.mb.node_0.soc.primary_die.scp.channel_manager

        if self.dut.get_dut_type() in (DeviceType.BIGFPGA, DeviceType.RVP):
            command = "sys plat_id"
            self.log.info(f"Submitting {command}\n")
            core_com_channel.write_line(write_string=command)
            try:
                command_response_cli=connection.get_current_channel().read_until(key="get_plat_id_comp", timeout_seconds=30)
            except Exception as e:
                self.log.error(f"Error reading Platform ID: {e}")
                self.test_notify(step="AVS Read volt.", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False 
            matches = re.search("Platform ID: PLATFORM_RVP_EVT_SILICON\n", command_response_cli)
            matches2 = re.search("Platform ID: PLATFORM_FPGA_LARGE_RVP\n", command_response_cli)
            self.log.info(f"matches: {matches}\n")
            if not matches:
                if not matches2:
                    self.log.warning(f"Platform ID not RVP_EVT_SILICON or PLATFORM_FPGA_LARGE_RVP: {command_response_cli}")
                    self.test_notify(step="Platform ID not RVP_EVT_SILICON or PLATFORM_FPGA_LARGE_RVP - no VRs", msg="", _is_error=False)
                    self.dut.teardown()
                    time.sleep(30)
                    return True        

        commands = ["avs avs_read 0 0 0", "avs avs_read 0 1 0", "avs avs_read 1 0 0", "avs avs_read 1 1 0", "avs avs_read 2 0 0", "avs avs_read 2 1 0", "avs avs_read 3 0 0", "avs avs_read 3 1 0"]
        for command in commands:
            self.log.info(f"Submitting {command}\n")
            core_com_channel.write_line(write_string=command)
            try:
                command_response_cli=connection.get_current_channel().read_until(key="avs_cli_comp", timeout_seconds=30)
            except Exception as e:
                self.log.error(f"Error reading AVS volt. Die0: {e}")
                self.test_notify(step="AVS Read volt.", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False 
            matches = re.search(".*AVS volt\. = ([0-9|\.]+)\n", command_response_cli)
            val = float(matches.group(1))
            if not (0.01 < val < 1.5):
                self.log.error(f"AVS voltage not in range 0.01V - 1.5V: {val}")
                self.test_notify(step="AVS volt. out of range", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            self.log.info(f"matches: {matches}")
            self.log.info(f"AVS voltage is: {val}")

        self.log.info(f"Reading AVS current Die0 . . .\n")
        commands = ["avs avs_read 0 0 2", "avs avs_read 0 1 2", "avs avs_read 1 0 2", "avs avs_read 1 1 2", "avs avs_read 2 0 2", "avs avs_read 2 1 2", "avs avs_read 3 0 2", "avs avs_read 3 1 2"]
        for command in commands:
            self.log.info(f"Submitting {command}\n")
            core_com_channel.write_line(write_string=command)
            try:
                command_response_cli=connection.get_current_channel().read_until(key="avs_cli_comp", timeout_seconds=30)
            except Exception as e:
                self.log.error(f"Error reading AVS current Die0: {e}")
                self.test_notify(step="AVS Read current", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            matches = re.search(".*AVS current = ([0-9|\.]+)\n", command_response_cli)
            val = float(matches.group(1))
            if (val > 497):
                self.log.error(f"AVS current greater than max of 497A {val}")
                self.test_notify(step="AVS curr. out of range", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            self.log.info(f"matches: {matches}")
            self.log.info(f"AVS current is: {val}")            

        self.log.info(f"Reading AVS temperature Die0 . . .\n")
        commands = ["avs avs_read 0 0 3", "avs avs_read 0 1 3", "avs avs_read 1 0 3", "avs avs_read 1 1 3", "avs avs_read 2 0 3", "avs avs_read 2 1 3", "avs avs_read 3 0 3", "avs avs_read 3 1 3"]
        for command in commands:
            self.log.info(f"Submitting {command}\n")
            core_com_channel.write_line(write_string=command)
            try:
                command_response_cli=connection.get_current_channel().read_until(key="avs_cli_comp", timeout_seconds=30)
            except Exception as e:
                self.log.error(f"Error reading AVS temperature Die0: {e}")
                self.test_notify(step="AVS Read temperature", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            matches = re.search(".*AVS temperature_dC = ([0-9|\.]+)\n", command_response_cli)
            val = float(matches.group(1))
            if (val > 115):
                self.log.error(f"AVS temperature > max of 115C {val}")
                self.test_notify(step="AVS temperature out of range", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            self.log.info(f"matches: {matches}")
            self.log.info(f"AVS temperature is: {val}")

        self.test_notify(step="AVS read Die0", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)
        return True
