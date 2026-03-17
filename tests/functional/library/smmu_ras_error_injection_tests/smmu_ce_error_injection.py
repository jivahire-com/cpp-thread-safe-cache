# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests SMMU CE (Correctable Error) Error Injection on SCP.
"""
import time
import sys
import os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest


# Class name must match file name for Robot Framework Library usage
class smmu_ce_error_injection(EchoFallsBaseTest):
    """
    SMMU CE Error Injection Test - SOC specific test.

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
        name: str = "SMMU_CE_Error_Injection_Test",
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

    def smmu_ce_error_injection(self) -> bool:
        """
        SMMU CE (Correctable Error) Error Injection test:
            1. Setup the Test.
            2. Open SCP channel and wait for ScpHeartBeat completion.
            3. Inject TCU RPSS 0 TMU HTTU RAM CE error via ap_mem_write command.
            4. Wait for RAS Record from SMMU TCU.
            5. Wait for ScpHeartBeat to confirm system is still running.
            6. Teardown Test.
        """
        self.log.info("Running SMMU CE (Correctable Error) Error Injection test . . .")

        self.dut.setup()

        scp_channel = self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()

        # Ensure the host config file used alongside this test has SCP connection defined
        assert scp_channel is not None

        # Open SCP channel
        scp_channel.open()

        if not scp_channel.is_open():
            self.log.error("Failed to open SCP channel")
            self.test_notify(step="Open_SCP_Channel", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        # Wait for ScpHeartBeat completion message
        self.log.info("Waiting for ScpHeartBeat completion message . . .")
        try:
            scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            scp_channel.close()
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        # Inject TCU RPSS 0 TMU HTTU RAM CE error
        self.log.info("Injecting TCU RPSS 0 TMU HTTU RAM CE error . . .")
        command = "mem ap_mem_write 0x5008060040 0x42000502"
        self.log.info(f"Submitting command: {command}")

        try:
            scp_channel.write_line(write_string=command)
            scp_channel.read_until(key="RAS Record from smmu_tcu", timeout_seconds=60)
        except Exception as e:
            self.log.error(f"Error in injecting smmu CE error: {e}")
            scp_channel.close()
            self.test_notify(step="SMMU_CE_Error_Injection", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        # Wait for ScpHeartBeat to confirm system is still running
        self.log.info("Waiting for ScpHeartBeat to confirm system stability . . .")
        try:
            scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=60)
        except Exception as e:
            self.log.error(f"Error reading ScpHeartBeat after error injection: {e}")
            scp_channel.close()
            self.test_notify(step="ScpHeartBeat_After_Injection", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        # Test completed successfully
        self.log.info("SMMU CE (Correctable Error) Error Injection test completed successfully")
        scp_channel.close()
        self.test_notify(step="SMMU_CE_Error_Injection", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)

        return True
