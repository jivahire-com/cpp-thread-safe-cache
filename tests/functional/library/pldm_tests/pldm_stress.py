"""
PLDM API Library
Set of common functions across different test cases and test flows

Copyright (C) Microsoft Corporation. All rights reserved.
Confidential and Proprietary.
"""

import os
import sys
import threading
import time

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "kng_pythia_libs"))

from kng_pythia_test_setup import KngPythiaTestSetup
from library.pldm_tests.pldm_common import pldm_common
from library.pldm_tests.pldm_sensors_effecters import pldm_sensors_effecters
from library.pldm_tests.pldm_platform import pldm_platform
from pathlib import Path
from RscmHelperLibrary import RscmHelperLibrary


class pldm_stress(pldm_common):
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
        name: str = "pldm_stress",
        number: str = "NaN",
        workspace_config: Path | str = None,
        default_log_home: str = None,
        fw_payload_path: str = None,
        dut_platform: str = None,
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
        self.sensors_effecters = pldm_sensors_effecters(
            name,
            number,
            workspace_config,
            default_log_home,
            fw_payload_path,
            dut_platform,
            host_config,
            host_name,
        )
        self.platform = pldm_platform(
            name,
            number,
            workspace_config,
            default_log_home,
            fw_payload_path,
            dut_platform,
            host_config,
            host_name,
        )
        self.timeout_minutes = None
        self.timed_out = None
        self.timer = None

    def setup(self):
        setup_step_1 = super().setup(keep_pldm_service_active=True)
        if not setup_step_1:
            return False
        setup_step_2 = self.sensors_effecters.setup_by_copy(
            self.dut,
            self.log,
            self.bmc_cli,
            self.core_mcp_channel,
            self.mctp_eids,
        )
        if not setup_step_2:
            return False
        setup_step_3 = self.platform.setup_by_copy(
            self.dut, self.log, self.bmc_cli, self.core_mcp_channel, self.mctp_eids
        )
        if not setup_step_3:
            return False
        return True

    def teardown(self):
        return super().teardown()

    def __disable_pldm_service_on_bmc(self):
        result, _, _ = self._bmc_execute_command(
            command="systemctl stop xyz.openbmc_project.pldmd.service",
            retry_delay=5,
            max_retries=20,
            sudo_mode=True
        )
        if result != 0:
            self.log.info("Failed to stop pldmd service on BMC")
            return False
        self.pldmd_stopped = True
        return True

    def __enable_pldm_service_on_bmc(self):
        result, _, _ = self._bmc_execute_command(
            command="systemctl start xyz.openbmc_project.pldmd.service",
            sudo_mode=True,
        )
        if result != 0:
            self.log.info("Failed to start pldmd service on BMC")
            return False
        self.pldmd_stopped = False
        return True

    def __log_failure(self, step_name: str, iteration: int) -> bool:
        self.log.error(
            f"Test failed at step '{step_name}' during iteration {iteration + 1}"
        )
        return False

    def __timeout_handler(self):
        self.timed_out = True

    def __raise_timeout_error(self):
        raise TimeoutError(
            f"{self.timeout_minutes} minutes have passed. Ending stress tests session."
        )

    def testcase_pldm_stress_test(self, timeout_minutes: int = 30) -> bool:
        """
        Test case: PLDM Stress Test
        Steps (go through the following steps in a loop until timeout is reached):
        1. Disable PLDM service on BMC
        2. Check numeric sensor
        3. Check state sensor
        4. Check numeric effecter
        5. Check state effecter
        6. Enable PLDM service on BMC
        7. Send PE CPER Small (disabled due to bug: https://azurecsi.visualstudio.com/Dev/_workitems/edit/3191148)
        8. Send PPE OEM Medium
        9. Send PPE CPER Medium
        10. Send PPE OEM Large
        11. Send PPE CPER Large
        """
        self.log.info("Test PLDM Stress Test Started")

        self.timeout_minutes = timeout_minutes
        self.timed_out = False
        self.timer = threading.Timer(self.timeout_minutes * 60, self.__timeout_handler)

        start_time = time.time()
        self.timer.start()
        try:
            i = 0
            while True:
                self.log.info(f"Iteration {i + 1} started")

                if self.__disable_pldm_service_on_bmc() is False:
                    return self.__log_failure("disable_pldm_service_on_bmc", i)

                if not self.sensors_effecters.testcase_01_set_numeric_sensor_enable():
                    return self.__log_failure(
                        "testcase_01_set_numeric_sensor_enable", i
                    )

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.sensors_effecters.testcase_03_get_numeric_sensor_reading():
                    return self.__log_failure(
                        "testcase_03_get_numeric_sensor_reading", i
                    )

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.sensors_effecters.testcase_05_set_state_sensor_enables():
                    return self.__log_failure("testcase_05_set_state_sensor_enables", i)

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.sensors_effecters.testcase_07_get_state_sensor_reading():
                    return self.__log_failure("testcase_07_get_state_sensor_reading", i)

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.sensors_effecters.testcase_09_set_numeric_effecter():
                    return self.__log_failure("testcase_09_set_numeric_effecter", i)

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.sensors_effecters.testcase_11_get_numeric_effecter():
                    return self.__log_failure("testcase_11_get_numeric_effecter", i)

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.sensors_effecters.testcase_13_set_state_effecter_states():
                    return self.__log_failure(
                        "testcase_13_set_state_effecter_states", i
                    )

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.sensors_effecters.testcase_15_get_state_effecter_states():
                    return self.__log_failure(
                        "testcase_15_get_state_effecter_states", i
                    )

                if self.timed_out:
                    self.__raise_timeout_error()

                if self.__enable_pldm_service_on_bmc() is False:
                    return self.__log_failure("enable_pldm_service_on_bmc", i)
                
                # Test case disabled due to bug: https://azurecsi.visualstudio.com/Dev/_workitems/edit/3191148,
                # this will be re-enabled once bug is fixed
                # if not self.platform.testcase1_pldm_send_pe_cper_small():
                #     return self.__log_failure(
                #         "testcase1_pldm_send_pe_cper_small", i
                #     )
                #
                # if self.timed_out:
                #     self.__raise_timeout_error()

                if not self.platform.testcase2_pldm_send_ppe_oem_medium():
                    return self.__log_failure("testcase2_pldm_send_ppe_oem_medium", i)

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.platform.testcase3_pldm_send_ppe_cper_medium():
                    return self.__log_failure("testcase3_pldm_send_ppe_cper_medium", i)

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.platform.testcase4_pldm_send_ppe_oem_large():
                    return self.__log_failure("testcase4_pldm_send_ppe_oem_large", i)

                if self.timed_out:
                    self.__raise_timeout_error()

                if not self.platform.testcase5_pldm_send_ppe_cper_large():
                    return self.__log_failure("testcase5_pldm_send_ppe_cper_large", i)

                if self.timed_out:
                    self.__raise_timeout_error()

                self.log.info(f"Iteration {i + 1} completed successfully")
                i += 1

        except TimeoutError as e:
            self.log.info(f"Caught exception: {e}")

        finally:
            self.timer.cancel()

        elapsed_time = time.time() - start_time
        self.log.info(f"Test PLDM Stress Test Completed in {elapsed_time:.2f} seconds")
        return True
