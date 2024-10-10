# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests the CDED POR'd Virtual Pipeline configuration(s) via UART.
"""
import time
import sys, os
from pathlib import Path

sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))
from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup

from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest

# Class name must match file name for Robot Framework Library usage
class cded_pipeline_init_test(EchoFallsBaseTest):

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
    
    def cded_pipeline_init_test(self, required_logs):
        """
        CDED pipeline init test:
            1. Setup the Test.
            2. Look for the required log strings.
            3. Teardown Test.
        """
        self.log.info("Running CDED POR'd Virtual Pipeline configuration(s) Init test . . .")
        self.dut.setup()

        if (self.dut.get_dut_type() == DeviceType.BIGFPGA):
            KngPythiaTestSetup.reset_fpga_sideload_testfw(self.dut, self.log)
            time.sleep(30)
            exp_num_lines=1400
        
        # Ensure the host config file used alongside this test has these connections defined.
        assert self.dut.mb.node_0.soc.primary_die.sdm_cded.channel_manager is not None
        self.dut.mb.node_0.soc.primary_die.sdm_cded.channel_manager.get_current_channel().open()
        assert self.dut.mb.node_0.soc.primary_die.sdm_cded.channel_manager.get_current_channel().is_open()
        exp_num_lines=120

        self.log.info("Reading CDED UART for required logs . . .")
        cded_lines = ' '.join(KngPythiaTestIF.read_from_uart(self, connection=self.dut.mb.node_0.soc.primary_die.sdm_cded.channel_manager, num_lines=exp_num_lines))

        self.dut.mb.node_0.soc.primary_die.sdm_cded.channel_manager.get_current_channel().close()

        # Check for missing logs
        missing_logs = [log for log in required_logs if log not in cded_lines]

        if missing_logs:
            self.log.error(f"Missing required logs: {', '.join(missing_logs)}")
            test_result = False
        else:
            self.log.info("All required logs found.")
            test_result = True

        self.test_notify(step="CDED PORT Virtual Pipeline configuration(s) Init test", msg="Test Done", _is_error=not(test_result))
        self.dut.teardown()

        return test_result
