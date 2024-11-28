# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests the Full Boot Chain HeartBeat from SDM and CDED via UART.
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class fbc_sdm_cded_heart_beat_single_die(EchoFallsBaseTest):

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
        name: str = "FBC_SDM_CDED_HeartBeat_Die0",
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
    
    def fbc_sdm_cded_heart_beat_single_die(self):
        """
        SDM/CDED heartbeat test:
            1. Setup the Test.
            2. Look for the SDM HeartBeat.
            3. Look for the CDED HeartBeat.
            4. Teardown Test.
        """
        if self.dut.get_dut_type() != DeviceType.BIGFPGA:
            self.log.warning("Bypassing test: Supported only on BIGFPGA.")
            return True  # Returning true to avoid blocking pipelines

        self.log.info("Running SDM/CDED heartBeat test . . .")

        sdm_connection = self.dut.mb.node_0.soc.primary_die.sdm.channel_manager
        sdm_cded_connection = self.dut.mb.node_0.soc.primary_die.sdm_cded.channel_manager
        scp_connection = self.dut.mb.node_0.soc.primary_die.scp.channel_manager

        # Ensure the host config file used alongside this test has these connections defined.
        assert sdm_connection is not None
        assert sdm_cded_connection is not None
        assert scp_connection is not None

        self.dut.setup()
        
        scp_channel = scp_connection.get_current_channel()
        scp_channel.open()
        if not scp_channel.is_open():
            self.dut.teardown()
            return False

        try:
            self.log.info("Reading from SCP till HeartBeat\n")
            scp_channel.read_until(key="HeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading SCP UART: {e}")
            self.dut.teardown()
            return False

        # Execute AFM scripts to map SDM and CDED-SDM connections
        KngPythiaTestSetup.execute_accel_afm_seq(self.dut.mb.node_0.soc.primary_die.scp.debugger, 2, 2)
        KngPythiaTestSetup.execute_accel_afm_seq(self.dut.mb.node_0.soc.primary_die.scp.debugger, 0, 2)

        # Open SDM connection
        sdm_channel = sdm_connection.get_current_channel()
        sdm_channel.open()
        if not sdm_channel.is_open():
            self.dut.teardown()
            return False

        # Open CDED-SDM connection
        sdm_cded_channel = sdm_cded_connection.get_current_channel()
        sdm_cded_channel.open()
        if not sdm_cded_channel.is_open():
            self.dut.teardown()
            return False

        self.log.info("Reading SDM UART for HeartBeat . . .")
        try:
            sdm_channel.read_until(key="SdmHeartBeat", timeout_seconds=30)
        except Exception as e:
            self.log.error(f"Error reading SDM UART: {e}")
            sdm_channel.close()
            self.test_notify(step="HeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        self.log.info("Reading CDED-SDM UART for HeartBeat . . .")
        try:
            sdm_cded_channel.read_until(key="SdmHeartBeat", timeout_seconds=30)
        except Exception as e:
            self.log.error(f"Error reading CDED_SDM UART: {e}")
            sdm_cded_channel.close()
            self.test_notify(step="HeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            return False

        # Close all channels after reading heartbeats
        sdm_channel.close()
        sdm_cded_channel.close()
        scp_channel.close()
        self.test_notify(step="HeartBeat", msg="Test Done", _is_error=False)
        self.dut.teardown()

        return True
