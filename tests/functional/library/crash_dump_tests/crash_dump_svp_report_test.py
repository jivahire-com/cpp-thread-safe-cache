# Copyright (c) Microsoft Corporation. All rights reserved.

"""
- Python based Pythia 2.0 Test.
- Tests Crash Dump Die0.
"""
import time
import sys, os, subprocess, json, shutil
from pathlib import Path


sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'kng_pythia_libs'))

from kng_pythia_test_if import KngPythiaTestIF
from kng_pythia_test_setup import KngPythiaTestSetup
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
import re


# Class name must match file name for Robot Framework Library usage
class crash_dump_svp_report_test(EchoFallsBaseTest):
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
        name: str = "crash_dump_test",
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
    
    def crash_dump_svp_report_test(self):
        """
        Crash Dump svp report test:
            1. Setup the Test.
            2. Executes Crash Dump cli command.
            3. Teardown Test.
            4. Check Crash dump file exist
            5. Parser CD file
        """

        REPO_APP_ROOT_DIR = os.environ.get("REPO_APP_ROOT")
        if REPO_APP_ROOT_DIR is None:
            self.log.info("REPO_APP_ROOT environment variable not set")
            return False

        self.log.info(f"REPO_APP_ROOT_DIR directory: {REPO_APP_ROOT_DIR}")
        crashdump_file_dir = os.path.join(
            REPO_APP_ROOT_DIR,
            ".svp_simulator",
            "crashdump"
        )

        if os.path.exists(crashdump_file_dir) and os.listdir(crashdump_file_dir):
            for file_name in os.listdir(crashdump_file_dir):
                file_path = os.path.join(crashdump_file_dir, file_name)
                os.remove(file_path)

        self.log.info("Running Crash Dump tests on Die0  . . .")
        self.dut.setup()

        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning("Device type is bigFPGA. Performing an additional OOB reset ...")
            KngPythiaTestSetup.fpga_oob_reset(self.log)

        scp_channel=self.dut.mb.node_0.soc.primary_die.scp.channel_manager.get_current_channel()
        # Open SCP channel
        scp_channel.open()
        if not scp_channel.is_open():
            scp_channel.close()
            self.test_notify(step="Open_Channels", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        try:
            # Wait for ScpHeartBeat Completion message and enter commands
            self.log.info("Waiting for ScpHeartBeat  Msg")
            scp_channel.read_until(key="ScpHeartBeat", timeout_seconds=900)
        except Exception as e:
            self.log.error(f"Error reading self.dut.mb.node_0.soc.primary_die.scp.channel_manager UART: {e}")
            self.test_notify(step="ScpHeartBeat", msg="Test Fail", _is_error=True)
            self.dut.teardown()
            time.sleep(30)
            return False

        command = "crashdump trig_except"
        self.log.info(f"Submitting {command}\n")
        scp_channel.write_line(write_string=command)
        try:
            scp_channel.read_until(key="crash_dump_svp_probe: Die0 Core1: Full dump", timeout_seconds=30)
        except Exception as e:
            self.log.error(f"Error Crash Dump svp report. Die0: {e}")
            self.test_notify(step="Crash Dump.", msg="Test Fail", _is_error=True)
            scp_channel.close()
            self.dut.teardown()
            time.sleep(30)
            return False

        # Close connection to SCP
        scp_channel.close()
        # Wait for .dmp ready
        time.sleep(5)

        working_dir = os.path.dirname(crashdump_file_dir)
        dmp_files = [f for f in os.listdir(crashdump_file_dir) if f.endswith('.dmp')]
        if dmp_files:
            for dmp_file in dmp_files:
                crashdump_file = os.path.join(crashdump_file_dir, dmp_file)
                self.log.info(f"Found .dmp file: {crashdump_file}")
                output_file = os.path.splitext(crashdump_file)[0] + "_report.json"

                if not os.path.exists(crashdump_file):
                    self.log.error(f"Crash file does not exist: {crashdump_file}")
                    self.dut.teardown()
                    return False

                cmd = [
                    "base_cd_analyze",
                    "-c", crashdump_file,
                    "-o", output_file
                ]

                try:
                    subprocess.run(cmd, cwd=working_dir, capture_output=True, text=True, check=True)
                    self.log.info(f"Crash dump CD file parser succeeded for {crashdump_file}")
                except subprocess.CalledProcessError as e:
                    self.log.error(f"Crash dump CD file parser error for {crashdump_file}: {e.stderr}")
                    self.dut.teardown()
                    return False

                with open(output_file, 'r') as f:
                    data = json.load(f)

                product_id = data['Product_Info'].get('Product_Id')
                crash_core_id = data['Crash_Info'].get('Crash_Core_Id')
                crash_error_code = data['Crash_Info'].get('Crash_Error_Code')
                if product_id != "0x92":
                    self.log.info(f"Expected Product_Id '0x92', but got {product_id}")
                    self.dut.teardown()
                    return False

                # mcp = 0x0
                if crash_core_id == "0x0" and crash_error_code != "0x80380007":
                    self.log.error(
                        f"Expected Crash_Error_Code '0x80380007', but got Crash_Error_Code '{crash_error_code}'"
                    )
                # scp = 0x1
                if crash_core_id == "0x1" and crash_error_code != "0x80380005":
                    self.log.error(
                        f"Expected Crash_Error_Code '0x80380005', but got Crash_Error_Code '{crash_error_code}'"
                    )
        else:
            self.log.info("No .dmp files found in the crashdump directory.")
            self.dut.teardown()
            return False
        
        self.test_notify(step="Crash Dump svp report Die0", msg="Test Done", _is_error=False)
        self.dut.teardown()
        time.sleep(30)
        return True
