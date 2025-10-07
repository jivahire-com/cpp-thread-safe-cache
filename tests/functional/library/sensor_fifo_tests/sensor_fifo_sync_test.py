# Copyright (c) Microsoft Corporation. All rights reserved.
"""
sensor_fifo_sync_test.py - Python based Pythia 2.0 Test.
Tests that check for sensor fifo sync behavior.
"""

import re
import sys
import os
import time
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent / "kng_pythia_libs"))
sys.path.append(str(Path(__file__).parent.parent / "common"))

from RscmHelperLibrary import RscmHelperLibrary  # noqa: E402
from serial_utils import SerialUtility  # noqa: E402
from pythia.tdk.echofalls.constants.dut_types import DeviceType  # noqa: E402
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest  # noqa: E402

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend(
    [
        pylibs_dir,
        current_dir,
    ]
)


class sensor_fifo_sync_test(EchoFallsBaseTest):

    def __init__(
        self,
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        host_config: Path | str = None,
        name: str = "SensorFifoSyncTest",
        number: str = "NaN",
        dut_platform: str = "KingsgateSVP",
        host_name: str | None = None,
    ):
        super().__init__(
            name=name,
            number=number,
            workspace_config=workspace_config,
            default_log_home=default_log_home,
            fw_payload_path=fw_payload_path,
            dut_platform=dut_platform,
            host_config=host_config,
            host_name=host_name,
        )
        print("\n\n\nBPK sensor_fifo_sync_test init called\n\n\n")
        self.log.info("\n\n\nBPK sensor_fifo_sync_test init called\n\n\n")
        self.serial_util = None

    def check_all_sensors_enabled(self, response: str) -> bool:
        """
        Check if all sensors are enabled based on the CLI response.

        Args:
            response: The CLI command response string

        Returns:
            True if all sensors are enabled, False otherwise
        """
        if not response:
            return False

        pattern = r"Fifo ID = (\d+):.*?enabled = (\w+)"
        matches = re.findall(pattern, response, re.DOTALL)

        # Return boolean AND of all enabled status
        all_enabled = True
        for fifo_id, enabled_status in matches:
            is_enabled = enabled_status.lower() == "true"
            all_enabled = all_enabled and is_enabled
            self.log.info(f"Fifo ID {fifo_id}: enabled = {enabled_status}")

        self.log.info(f"All enabled {all_enabled}")
        return all_enabled

    def sync_test(self, die: int) -> bool:
        """
        Functional Test that validates sensor fifos enablement sync correctly.
        """
        self.log.info(f"Running SENSOR FIFO Sync TEST on die {die}")

        die = int(die)
        if die not in [0, 1]:
            raise ValueError(f"Unexpected Die {die} Expected [0, 1]")
        try:
            self.log.info("Setting up DUT...")
            self.dut.setup()

            if self.dut.get_dut_type() not in [DeviceType.RVP, DeviceType.SVP]:
                self.log.error(
                    f"Test not supported on this DUT type {self.dut.get_dut_type().name} "
                    f"not in [{DeviceType.RVP.name}, {DeviceType.SVP.name}]. Skipping test."
                )
                return False

            if self.dut.get_dut_type() == DeviceType.RVP:
                self.log.warning("Device type is RVP. Performing SoC Reset ...")
                cred_path = os.environ.get("SECURE_FILE_PATH")
                creds = self.load_credentials_from_yaml(cred_path)
                rscm_helper = RscmHelperLibrary(
                    rm_host="172.29.89.33",
                    bmc_host="172.17.0.97",
                    rm_user=creds["RM_USER"],
                    rm_password=creds["RM_PASSWORD"],
                    bmc_user=creds["BMC_USER"],
                    bmc_password=creds["BMC_PASSWORD"],
                )  # Fill in real host if available
                rscm_helper.rscm_soc_reset()

            # This test will:
            # 1. Connect to the SCP amd MCP on the specified die
            # 2. Query the sensor fifo properties via CLI commands on the SCP
            # 3. Check the output to see if all the sensors are enabled
            # 4. If they are not all enabled sleep for a bit and go back to step 2
            # 5. If they are all enabled proceed to the next step
            # 6. Query the sensor fifo properties via CLI commands on the MCP
            # 7. Check the output to see if all the sensors are enabled
            # 8. If they are not all enabled fail the test.

            # Step 1: Initialize communication channels

            pythia_die = (
                self.dut.mb.node_0.soc.primary_die
                if die == 0
                else self.dut.mb.node_0.soc.secondary_die
            )

            self.serial_util = SerialUtility(
                scp_channel=pythia_die.scp.channel_manager.get_current_channel(),
                mcp_channel=pythia_die.mcp.channel_manager.get_current_channel(),
                logger=self.log,
                dut=self.dut,
            )

            self.log.info("Serial Util created successfully")

            if not self.serial_util.wait_for_scp_mcp_heartbeat():
                self.log.error("Failed to detect SCP and MCP heartbeat")
                return False

            # Step 2-5: Query SCP sensor fifo properties and wait for all to be enabled
            max_retries = 30  # Maximum number of retries
            retry_delay = 5.0  # Delay between retries in seconds

            for attempt in range(max_retries):
                self.log.info(
                    f"Querying SCP sensor fifo properties (attempt {attempt + 1}/{max_retries})"
                )

                # Query sensor fifo properties on SCP
                scp_response = self.serial_util.run_command_on_scp(
                    command="snsrfifo lprop", read_until_key="Ok"
                )

                if self.check_all_sensors_enabled(scp_response):
                    self.log.info(
                        "All SCP sensor fifos are enabled, proceeding to MCP check"
                    )
                    break
                else:
                    self.log.info(
                        f"Not all SCP sensor fifos enabled yet, retrying in {retry_delay}s..."
                    )
                    time.sleep(retry_delay)
            else:
                self.log.error(
                    "Timeout: Not all SCP sensor fifos became enabled within retry limit"
                )
                return False

            # Step 6-8: Query MCP sensor fifo properties and verify all are enabled
            self.log.info("Querying MCP sensor fifo properties")
            mcp_response = self.serial_util.run_command_on_mcp(
                command="snsrfifo lprop", read_until_key="Ok"
            )

            if not self.check_all_sensors_enabled(mcp_response):
                self.log.error("Not all MCP sensor fifos are enabled - test failed")
                return False

            self.log.info("All MCP sensor fifos are enabled - test passed")
            return True

        except Exception as e:
            self.log.error(f"Error during test execution: {str(e)}")
            return False
        finally:
            # Cleanup resources if needed
            if hasattr(self, "serial_util") and self.serial_util:
                self.serial_util = None

    @staticmethod
    def load_credentials_from_yaml(path):
        import yaml

        with open(path, "r") as f:
            data = yaml.safe_load(f)
        return data
