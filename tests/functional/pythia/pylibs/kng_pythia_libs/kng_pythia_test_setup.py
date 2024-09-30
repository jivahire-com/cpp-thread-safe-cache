"""
- Python based Pythia 2.0 Test.
- This module contains setup, programming and teardown related utility functions for pythia functional tests.
"""

import os
import time
from pathlib import Path

class KngPythiaTestSetup():

    @staticmethod
    def reset_fpga_sideload_testfw(dut, log):

        script_path = path_to_fpga_launch_scripts = os.path.join(os.getcwd().split('.testlogs')[0],"tests/functional/pythia/cmm/")
        script_name = "PythiaLaunchFPGA.cmm"
        script = Path(r"{}".format(os.path.join(script_path, script_name)))

        log.info("Resetting FPGA and sideloading Test Firmware . . .")
        log.info(f"Calling HSP CMM Script to reset and load ProdFW: {script}")
        hsp = dut.mb.node_0.soc.primary_die.get_core("hsp")
        hsp.debugger.execute_script(script)
        
        boot_time=90
        log.info(f"Waiting {boot_time} seconds for FPGA to boot . . .")
        time.sleep(boot_time)

        log.info("Resetting FPGA and launching ProdFW Done!")
