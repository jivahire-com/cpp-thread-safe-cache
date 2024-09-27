# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests AVS reads.
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
import re

# Class name must match file name for Robot Framework Library usage
class scp_avs_read_test(EchoFallsBaseTest):

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
    
    def avs_read_test(self):
        """
        AVS read test:
            1. Setup the Test.
            2. Executes AVS reads.
            3. Teardown Test.
        """
        self.log.info("Running AVS read tests on Die0  . . .")
        self.dut.setup()

        if (self.dut.get_dut_type() == DeviceType.BIGFPGA):
            # TODO: Task 2061224: AVS Read Functional Tests for RVP, with error checking for out of range values.
            #        KngPythiaTestSetup.reset_fpga_load_prodfw(self)
            #        time.sleep(30)
            self.log.info("TODO BIGFPGA/RVP AVS tests")
            self.dut.teardown()
            return True
        
        elif (self.dut.get_dut_type() == DeviceType.SVP):
            print("SVP device")
            core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
            core_com_channel.open()
            core_com_channel.is_open()

        else:
            self.log.error("Unsupported DUT type")
            self.dut.teardown()
            return False

        connection = self.dut.mb.node_0.soc.primary_die.scp.channel_manager

        commands = ["avs avs_read 0 0 0", "avs avs_read 0 1 0", "avs avs_read 1 0 0", "avs avs_read 1 1 0", "avs avs_read 2 0 0", "avs avs_read 2 1 0", "avs avs_read 3 0 0", "avs avs_read 3 1 0"]
        for command in commands:
            self.log.info(f"Submitting {command}\n") 
            core_com_channel.execute_command(command=command, command_args= "")
            command_response_cli=connection.get_current_channel().read_until(key="avs_cli_comp", timeout_seconds=30)
            matches = re.search(".+AVS volt\. = ([0-9|\.]+)\n", command_response_cli)
            val = float(matches.group(1))
            if val > 1.5:
                self.log.error(f"AVS voltage is > 1.5V: {val}")
                self.dut.teardown()
                return False
            self.log.info(f"matches: {matches}")
            self.log.info(f"AVS voltage is: {val}")

        commands = ["avs avs_read 0 0 2", "avs avs_read 0 1 2", "avs avs_read 1 0 2", "avs avs_read 1 1 2", "avs avs_read 2 0 2", "avs avs_read 2 1 2", "avs avs_read 3 0 2", "avs avs_read 3 1 2"]
        for command in commands:
            self.log.info(f"Submitting {command}\n") 
            core_com_channel.execute_command(command=command, command_args= "")
            command_response_cli=connection.get_current_channel().read_until(key="avs_cli_comp", timeout_seconds=30)

        commands = ["avs avs_read 0 0 3", "avs avs_read 0 1 3", "avs avs_read 1 0 3", "avs avs_read 1 1 3", "avs avs_read 2 0 3", "avs avs_read 2 1 3", "avs avs_read 3 0 3", "avs avs_read 3 1 3"]
        for command in commands:
            self.log.info(f"Submitting {command}\n") 
            core_com_channel.execute_command(command=command, command_args= "")
            command_response_cli=connection.get_current_channel().read_until(key="avs_cli_comp", timeout_seconds=30)

        self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel().close()
        self.dut.teardown()

        return True
