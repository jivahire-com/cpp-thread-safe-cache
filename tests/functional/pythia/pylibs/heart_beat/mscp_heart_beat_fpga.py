"""
- Python based Pythia 2.0 Test.
- Tests that the HeartBeat from the SCP core transmits via the UART.
"""

import time
import os
from pathlib import Path
from ..test_lib.base_mscp_test import BaseMSCPTest

# Class name must match file name for Robot Framework Library usage
class mscp_heart_beat_fpga(BaseMSCPTest):

    """
    :param name: Name of the test case
    :param workspace_config: Path to the workspace config file
    :param default_log_home: Path for log storage
    :param fw_payload_path: Path to the recipe payload
    :param dut_platform: Platform used during the test case execution
    :param host_config: Path to the host config file/directory
    """
    def __init__(
        self,
        name: str = "mscp_runtime_fw_heart_beat",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        host_config: Path | str = None,
    ):

        # Call parent class init
        super().__init__(
            name=name,
            workspace_config=workspace_config,
            default_log_home=default_log_home,
            fw_payload_path=fw_payload_path,
            host_config=host_config,
        )

    def test(self):
        """
        Test function:
            1. Setup the Test.
            2. Assert that a connection to SCP on DIE 0 can be established and open it.
            3. Look for the HeartBeat from the SCP.
            4. Teardown Test.
        """
        self.log.info("Running SCP heartBeat test . . .")

        self.dut.setup()

        # Wait 5 seconds for all the init steps to complete
        time.sleep(5)

        self.reset_fpga_load_prodfw(soc=self.dut.mb.node_0.soc)

        self.log.info("Reading SCP UART for HeartBeat . . .")
        scp_lines = ' '.join(self.read_and_log_lines(connection=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel(), num_lines=900))

        test_pass = True if ('HeartBeat' in scp_lines) else False

        self.test_notify(step="HeartBeat", msg="Test Done", _is_error=not(test_pass))
        self.dut.teardown()

        return test_pass

    def reset_fpga_load_prodfw(self, soc):

        if soc is None:
            raise RuntimeError("Restart SoC called on None instance")

        self.log.info("Resetting FPGA and launching ProdFW . . .")
        
        primary_die = soc.primary_die
        hsp = primary_die.get_core("hsp")
        scp = primary_die.get_core("scp")
        hsp_debugger = primary_die.get_core("hsp").debugger

        path_to_fpga_launch_scripts ="tests/functional/pythia/cmm/PythiaLaunchFPGA.cmm"
        hsp_load_prodfw_script_path = os.path.join(os.getcwd().split('.testlogs')[0],path_to_fpga_launch_scripts)
        hsp_load_prodfw_script = Path(r"{}".format(hsp_load_prodfw_script_path))

        self.log.info(f"Calling HSP CMM Script to reset and load ProdFW: {hsp_load_prodfw_script}")
        
        hsp_debugger.execute_script(hsp_load_prodfw_script)
        
        # Wait 30 seconds for the script to execute and load ProdFW
        time.sleep(30)