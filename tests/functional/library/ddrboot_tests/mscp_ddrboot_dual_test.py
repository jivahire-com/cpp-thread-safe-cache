# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests the HeartBeat from MSCP via UART.
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class mscp_ddrboot_dual_test(EchoFallsBaseTest):

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
        name: str = "MSCP_DDRBOOT_DualDie",
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
    
    def mscp_ddrboot_test(self):
        """
        MSCP DDRBoot test:
            1. Setup the Test.
            2. Look for the SCP DDRBoot key word.
            3. Teardown Test.
        """
        self.log.info("Running DDRBoot Test. . .")
        self.dut.setup()
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)
            
        core_com_channel=self.dut.mb.node_0.soc.secondary_die.scp.channel_manager.get_current_channel()
        mcp_com_channel=self.dut.mb.node_0.soc.secondary_die.mcp.channel_manager.get_current_channel()
        core_com_channel.open()
        mcp_com_channel.open()
        assert core_com_channel.is_open()
        assert  mcp_com_channel is not None
        try:
            self.log.info(f"Reading BDAT DONE")
            core_com_channel.read_until(key="BDAT DONE", timeout_seconds=1800)
            self.log.info(f"Reading SMBIOS 16 DONE")
            # Clear buffer and read until BDAT
            core_com_channel.read_until(key="SMBIOS 16 DONE", timeout_seconds=1800)
            self.log.info(f"Reading from SMBIOS 17 DONE")
            # Clear buffer and read until SMBIOS 17 DONE
            core_com_channel.read_until(key="SMBIOS 17 DONE", timeout_seconds=1800)
            self.log.info(f"Reading from HeartBeat")
            # Clear buffer and read until HeartBeat
            core_com_channel.read_until(key="HeartBeat", timeout_seconds=1800)
            mcp_com_channel.read_until(key="HeartBeat", timeout_seconds=1800)
        except Exception as e:
            core_com_channel.close()
            mcp_com_channel.close()
            self.log.error(f"Error during test execution: {e}")
            self.test_notify(step="Test Execution", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False
        finally:
            core_com_channel.close()
            mcp_com_channel.close()
            self.test_notify(step="DDRBoot Dual Test", msg="Test Done", _is_error=False)
            self.dut.teardown()
            time.sleep(30)
            return True
