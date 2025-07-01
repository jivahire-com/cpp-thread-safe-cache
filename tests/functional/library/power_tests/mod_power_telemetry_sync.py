# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Test to ensure that the power service is able to notify the
  power telemetry service of Sensor FIFOs being enabled
"""

import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class mod_power_telemetry_sync(EchoFallsBaseTest):

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
        name: str = "Power telemetry sync test",
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
    
    def power_telemetry_sync_test(self):
        """
        Power Module control loop CLI test:
            1. Setup the Test.
            2. Verify that the SoS Service has finished all boot phases
            3. Verify that no FifoIccSendFail errors are present
            4. Teardown Test.
        """
        self.log.info("Running Power telemetry sync test")
        self.dut.setup()
            
        core_com_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        core_com_channel.open()
        assert core_com_channel.is_open()

        # Wait for the SoS service to finish all boot phases
        lines = None
        try:
            self.log.info("Waiting for boot complete message on SCP UART")
            lines = core_com_channel.read_until(key="SOS boot completed", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="Boot complete", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        # Check for FifoIccSendFail errors in the SCP UART log. If no failures pass the test
        if lines is not None:
            if "FifoIccSendFail" in lines:
                self.log.error("FifoIccSendFail error found in SCP UART log")
                self.test_notify(step="Power module telemetry sync test", msg="Test Fail", _is_error=True)
                core_com_channel.close()
                self.dut.teardown()
                time.sleep(30)
                return False
            else:
                self.log.info("No FifoIccSendFail errors found in SCP UART log")
                self.test_notify(step="Power module telemetry sync test", msg="Test Pass", _is_error=False)
        else:
            self.log.error("No lines read from SCP UART channel")
            self.test_notify(step="Power module telemetry sync test", msg="Test Fail", _is_error=True)

        core_com_channel.close()
        self.test_notify(step="Power Telemetry sync test complete", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True