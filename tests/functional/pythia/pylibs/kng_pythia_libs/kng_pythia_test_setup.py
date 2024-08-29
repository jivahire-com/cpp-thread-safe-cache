"""
- Python based Pythia 2.0 Test.
- This module contains setup, programming and teardown related utility functions for pythia functional tests.
"""

import os
import time
from pathlib import Path

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

class KngPythiaTestSetup(EchoFallsBaseTest):

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
    
    def test(self):
        raise NotImplementedError("Abstract method test must be overridden")
    
    def reset_fpga_load_prodfw(self):
        
        hsp = self.dut.mb.node_0.soc.primary_die.get_core("hsp")

        path_to_fpga_launch_scripts = os.path.join(os.getcwd().split('.testlogs')[0],"tests/functional/pythia/cmm/PythiaLaunchFPGA.cmm")
        hsp_load_prodfw_script = Path(r"{}".format(path_to_fpga_launch_scripts))

        self.log.info("Resetting FPGA and launching ProdFW . . .")
        self.log.info(f"Calling HSP CMM Script to reset and load ProdFW: {hsp_load_prodfw_script}")
        hsp.debugger.execute_script(hsp_load_prodfw_script)
        
        boot_time=90
        self.log.info("Waiting {boot_time} seconds for FPGA to boot . . .")
        time.sleep(boot_time)
        self.log.info("Resetting FPGA and launching ProdFW Done!")
