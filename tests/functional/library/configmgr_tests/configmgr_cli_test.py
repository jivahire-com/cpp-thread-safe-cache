# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test that checks for Config Manager CLI List Command.
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

class configmgr_cli_test(EchoFallsBaseTest):

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
        name: str = "Configmgr_cli_test_list",
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
    
    def configmgr_cli_test(self):
        """ 
        Test function: 
            1. Setup the Test. 
            2. Wait for SCP to be up. Send 'List' command 
            3. Read response and check if test passed or failed based on response
            4. Teardown Test. 
        """
        self.log.info("Running Config Manager CLI command Test. . .")
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
            creds = self.load_credentials_from_yaml(cred_path)
            rscm_helper = RscmHelperLibrary(rm_host="172.29.89.33", bmc_host="172.17.0.97", rm_user=creds['RM_USER'], rm_password=creds['RM_PASSWORD'], bmc_user=creds['BMC_USER'], bmc_password=creds['BMC_PASSWORD'])  # Fill in real host if available
            rscm_helper.rscm_soc_reset()
            
        core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_channel.open()
        assert core_com_channel.is_open()
        
        try:
            self.log.info("Waiting for Heartbeat Msg")
            core_com_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        self.log.info("Submitting Config Manager List command . . .")
        command="cfg_mgr"
        core_com_channel.write_line(write_string=command)

        for attempt in range(2):
            command="cfg_mgr_list"
            self.log.info(f"Submitting LIST command {command} . . .") 
            core_com_channel.write_line(write_string=command)

            try:
                core_com_channel.read_until(key="System Knob Configuration Entities Completed", timeout_seconds=300)
                self.log.info("LIST command executed Successfully . . .")
            except Exception as e:
                self.log.error(f"Error reading SCP UART: {e}")
                core_com_channel.close()
                self.test_notify(step="Config Manager LIST Command", msg="Test Fail", _is_error=True)
                self.dut.teardown()
                time.sleep(30)
                return False
            
        core_com_channel.close()
        self.test_notify(step="Config Manager LIST Command", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True
    
    @staticmethod
    def load_credentials_from_yaml(path):
        import yaml
        with open(path, 'r') as f:
            data = yaml.safe_load(f)
        return data