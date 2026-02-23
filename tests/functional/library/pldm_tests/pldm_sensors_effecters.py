"""
PLDM API Library
Set of common functions across different test cases and test flows

Copyright (C) Microsoft Corporation. All rights reserved.
Confidential and Proprietary.
"""

import os
import re
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), "..", "kng_pythia_libs"))

from kng_pythia_test_setup import KngPythiaTestSetup
from library.pldm_tests.pldm_common import pldm_common
from library.pldm_tests.pldm_spec_parser import pldm_spec_parser
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
from pathlib import Path
from RscmHelperLibrary import RscmHelperLibrary
from collections import defaultdict


class pldm_sensors_effecters(pldm_common):
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
        name: str = "pldm_sensors_effecters",
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
        self.numeric_sensor_limit = 0
        self.state_sensor_limit = 0
        self.numeric_effecter_limit = 0
        self.state_effecter_limit = 0
        self.numeric_sensor_ids = defaultdict(list)
        self.state_sensor_ids = defaultdict(list)
        self.numeric_effecter_ids = defaultdict(list)
        self.state_effecter_ids = defaultdict(list)

    def setup(self):
        setup_status = super().setup()
        if setup_status is False:
            return False
        self.numeric_sensor_limit = self.pldm_spec.get_numeric_sensor_limit()
        self.state_sensor_limit = self.pldm_spec.get_state_sensor_limit()
        self.numeric_effecter_limit = self.pldm_spec.get_numeric_effecter_limit()
        self.state_effecter_limit = self.pldm_spec.get_state_effecter_limit()
        self.__parse_pdr_records()
        return True

    def setup_by_copy(self, dut, log, bmc_cli, core_mcp_channel, mctp_eid_list):
        setup_status = super().setup_by_copy(
            dut, log, bmc_cli, core_mcp_channel, mctp_eid_list
        )
        if setup_status is False:
            return False
        self.numeric_sensor_limit = self.pldm_spec.get_numeric_sensor_limit()
        self.state_sensor_limit = self.pldm_spec.get_state_sensor_limit()
        self.numeric_effecter_limit = self.pldm_spec.get_numeric_effecter_limit()
        self.state_effecter_limit = self.pldm_spec.get_state_effecter_limit()
        self.__parse_pdr_records()
        return True

    def teardown(self):
        return super().teardown()

    def __parse_pdr_records(self):
        for mctp_eid in self.mctp_eids:
            _, repo_info, _ = self._bmc_execute_command(
                f"pldmtool platform GetPDRRepositoryInfo -m {mctp_eid} -v"
            )
            match = re.search(r'"RecordCount":\s*(\d+)', repo_info)
            record_count = int(match.group(1)) if match else 0

            for handle in range(record_count):
                _, output, _ = self._bmc_execute_command(
                    f"pldmtool platform GetPDR -m {mctp_eid} -d {handle} -v"
                )

                if (
                    "Numeric Sensor PDR" in output
                    and len(self.numeric_sensor_ids[mctp_eid])
                    < self.numeric_sensor_limit
                ):
                    sid_match = re.search(r'"sensorID":\s*(\d+)', output)
                    if sid_match:
                        self.numeric_sensor_ids[mctp_eid].append(
                            int(sid_match.group(1))
                        )

                elif (
                    "State Sensor PDR" in output
                    and len(self.state_sensor_ids[mctp_eid]) < self.state_sensor_limit
                ):
                    sid_match = re.search(r'"sensorID":\s*(\d+)', output)
                    if sid_match:
                        self.state_sensor_ids[mctp_eid].append(int(sid_match.group(1)))

                elif (
                    "Numeric Effecter PDR" in output
                    and len(self.numeric_effecter_ids[mctp_eid])
                    < self.numeric_effecter_limit
                ):
                    eid_match = re.search(r'"effecterID":\s*(\d+)', output)
                    if eid_match:
                        self.numeric_effecter_ids[mctp_eid].append(
                            int(eid_match.group(1))
                        )

                elif (
                    "State Effecter PDR" in output
                    and len(self.state_effecter_ids[mctp_eid])
                    < self.state_effecter_limit
                ):
                    eid_match = re.search(r'"effecterID":\s*(\d+)', output)
                    if eid_match:
                        self.state_effecter_ids[mctp_eid].append(
                            int(eid_match.group(1))
                        )

                if (
                    len(self.numeric_sensor_ids[mctp_eid]) >= self.numeric_sensor_limit
                    and len(self.state_sensor_ids[mctp_eid]) >= self.state_sensor_limit
                    and len(self.numeric_effecter_ids[mctp_eid])
                    >= self.numeric_effecter_limit
                    and len(self.state_effecter_ids[mctp_eid])
                    >= self.state_effecter_limit
                ):
                    break

    def testcase_01_set_numeric_sensor_enable(self):
        """Test enabling/disabling numeric sensors with valid operational states"""
        self.log.info("Executing Test: SetNumericSensorEnable")

        if not self.mctp_eids:
            return False

        operational_state_enabled = '"SensorOperationalState": "Enabled"'
        operational_state_disabled = '"SensorOperationalState": "Disabled"'

        for mctp_id in self.mctp_eids:
            for numeric_sensor_id in self.numeric_sensor_ids[mctp_id]:
                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform SetNumericSensorEnable -m {mctp_id} -i {numeric_sensor_id} -o 1 -e 0 -v"
                )
                if "SUCCESS" not in stdout:
                    self.log.info("Failed to disable numeric sensor")
                    return False

                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform GetNumericSensorReading -m {mctp_id} -i {numeric_sensor_id} -r 0 -v"
                )
                if operational_state_disabled not in stdout:
                    self.log.info("Numeric sensor not disabled as expected")
                    return False

                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform SetNumericSensorEnable -m {mctp_id} -i {numeric_sensor_id} -o 0 -e 0 -v"
                )
                if "SUCCESS" not in stdout:
                    self.log.info("Failed to enable numeric sensor")
                    return False

                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform GetNumericSensorReading -m {mctp_id} -i {numeric_sensor_id} -r 0 -v"
                )
                if operational_state_enabled not in stdout:
                    self.log.info("Numeric sensor not enabled as expected")
                    return False

        return True

    def testcase_02_set_numeric_sensor_enable_error_case(self):
        """Test error handling for invalid sensor configurations"""
        self.log.info("Executing Test: SetNumericSensorEnableErrorCase")

        if not self.mctp_eids:
            return False

        # not available sensor ID
        na_sensor_id = 9999

        for mctp_id in self.mctp_eids:
            # Test invalid sensor ID
            _, _, stderr = self._bmc_execute_command(
                f"pldmtool platform SetNumericSensorEnable -m {mctp_id} -i {na_sensor_id} -o 0 -e 0 -v"
            )
            if "Error" not in stderr:
                self.log.info("Expected error not received for invalid sensor ID")
                return False

            # Invalid event msg enable option, expect failure
            # event msg enable = disable events(1)/enable events(2)/enable op events only(3)/enable state events only(4)
            for numeric_sensor_id in self.numeric_sensor_ids[mctp_id]:
                result, _, stderr = self._bmc_execute_command(
                    f"pldmtool platform SetNumericSensorEnable -m {mctp_id} -i {numeric_sensor_id} -o 0 -e 2 -v"
                )
                if "Error" not in stderr:
                    self.log.info(
                        "Expected error not received for invalid enable events"
                    )
                    return False

        return True

    def testcase_03_get_numeric_sensor_reading(self):
        """Test reading numeric sensor values"""
        self.log.info("Executing Test: GetNumericSensorReading")

        if not self.mctp_eids:
            return False

        expected_numeric_reading_strs = [
            '"SensorOperationalState": "Enabled"',
            '"SensorEventMessageEnable": ',
            '"PresentReading": ',
        ]

        for mctp_id in self.mctp_eids:
            for numeric_sensor_id in self.numeric_sensor_ids[mctp_id]:
                # Enable sensor and set test value
                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform SetNumericSensorEnable -m {mctp_id} -i {numeric_sensor_id} -o 0 -e 0 -v"
                )
                # self.__bmc_execute_command("pldm_dummy_num_sensor_set 100") # command not available in DH3's BMC
                if "SUCCESS" not in stdout:
                    self.log.info("Failed to enable numeric sensor")
                    return False

                # Read sensor value
                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform GetNumericSensorReading -m {mctp_id} -i {numeric_sensor_id} -r 0 -v"
                )

                if not all(x in stdout for x in expected_numeric_reading_strs):
                    self.log.info("Invalid sensor reading response")
                    return False
        return True

    def testcase_04_get_numeric_sensor_reading_error_case(self):
        """Test error handling for invalid sensor reads"""
        self.log.info("Executing Test: GetNumericSensorReadingErrorCase")

        if not self.mctp_eids:
            return False

        # not available sensor ID
        na_sensor_id = 9999

        for mctp_id in self.mctp_eids:
            result, _, stderr = self._bmc_execute_command(
                f"pldmtool platform GetNumericSensorReading -m {mctp_id} -i {na_sensor_id} -r 0 -v"
            )
            if "Error" not in stderr:
                self.log.info("Expected error not received for invalid sensor read")
                return False
        return True

    def testcase_05_set_state_sensor_enables(self):
        """Test configuration of state sensors"""
        self.log.info("Executing Test: SetStateSensorEnables")

        if not self.mctp_eids:
            return False

        op_state_sensor_enabled = '"sensorOpState[0]": "Sensor Enabled"'

        for mctp_id in self.mctp_eids:
            for state_sensor_id in self.state_sensor_ids[mctp_id]:
                # sensor id = 1 (registered sensor id),
                # event msg enable = no change (sensor doesn't support event generation)
                # composite sensor count = 1
                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform SetStateSensorEnables -m {mctp_id} -i {state_sensor_id} -c 1 -o 0x0000 -v"
                )
                if "SUCCESS" not in stdout:
                    self.log.info("Failed to enable state sensor")
                    return False

                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform GetStateSensorReadings -m {mctp_id} -i {state_sensor_id} -r 0 -v"
                )
                if op_state_sensor_enabled not in stdout:
                    self.log.info("State sensor not enabled as expected")
                    return False

        return True

    def testcase_06_set_state_sensor_enables_error_cases(self):
        """Test error handling for invalid state sensor configurations"""
        self.log.info("Executing Test: SetStateSensorEnablesErrorCases")

        if not self.mctp_eids:
            return False

        # not available sensor ID
        na_sensor_id = 9999

        for mctp_id in self.mctp_eids:
            # Test invalid sensor ID
            _, _, stderr = self._bmc_execute_command(
                f"pldmtool platform SetStateSensorEnables -m {mctp_id} -i {na_sensor_id} -c 1 -o 0x0000 -v"
            )
            if "Error" not in stderr:
                self.log.info("Expected error not received for invalid state sensor")
                return False
            # Test invalid event message enable options
            for state_sensor_id in self.state_sensor_ids[mctp_id]:
                _, _, stderr = self._bmc_execute_command(
                    f"pldmtool platform SetStateSensorEnables -m {mctp_id} -i {state_sensor_id} -c 1 -o 0x1000 -v"
                )
                if "Error" not in stderr:
                    self.log.info(
                        "Expected error not received for invalid event message"
                    )
                    return False
        return True

    def testcase_07_get_state_sensor_reading(self):
        """Test reading state sensor values"""
        self.log.info("Executing Test: GetStateSensorReading")

        if not self.mctp_eids:
            return False

        expected_output_strs = [
            '"sensorOpState[0]": "Sensor Enabled"',
            '"presentState[0]": ',
            '"previousState[0]": ',
            '"eventState[0]": ',
        ]
        for mctp_id in self.mctp_eids:
            for state_sensor_id in self.state_sensor_ids[mctp_id]:
                self._bmc_execute_command(
                    f"pldmtool platform SetStateSensorEnables -m {mctp_id} -c 1 -i {state_sensor_id} -o 0x0000 -v"
                )
                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform GetStateSensorReadings -m {mctp_id} -i {state_sensor_id} -r 0 -v"
                )
                if not all(x in stdout for x in expected_output_strs):
                    self.log.info("Invalid sensor reading response")
                    return False
        return True

    def testcase_08_get_state_sensor_reading_error_case(self):
        """Test error handling for invalid state sensor reads"""
        self.log.info("Executing Test: GetStateSensorReadingErrorCase")

        if not self.mctp_eids:
            return False

        # not available sensor ID
        na_sensor_id = 9999

        for mctp_id in self.mctp_eids:
            _, _, stderr = self._bmc_execute_command(
                f"pldmtool platform GetStateSensorReadings -m {mctp_id} -i {na_sensor_id} -r 0 -v"
            )
            if "Error" not in stderr:
                self.log.info(
                    "Expected error not received for invalid state sensor read"
                )
                return False
        return True

    def testcase_09_set_numeric_effecter(self):
        """Set numeric effecter value dynamically"""
        if not self.mctp_eids:
            return False

        data = 50
        size_map = {"UINT8": 1, "UINT16": 2, "UINT32": 4}

        for mctp_id in self.mctp_eids:
            for eff_id in self.numeric_effecter_ids[mctp_id]:

                _, out, _ = self._bmc_execute_command(
                    f"pldmtool platform GetNumericEffecterValue -m {mctp_id} -i {eff_id} -v"
                )

                match = re.search(r'"EffectorDataSize":\s*"([A-Z0-9]+)"', out)
                if not match:
                    self.log.error("Failed to get effecter data size")
                    return False

                effecter_data_size = size_map.get(match.group(1), 2)

                _, out, _ = self._bmc_execute_command(
                    f"pldmtool platform SetNumericEffecterValue -m {mctp_id} -i {eff_id} "
                    f"-s {effecter_data_size} -d {data} -v"
                )
                if "SUCCESS" not in out:
                    self.log.error("Failed to set state numeric effecter value")
                    return False

                _, out, _ = self._bmc_execute_command(
                    f"pldmtool platform GetNumericEffecterValue -m {mctp_id} -i {eff_id} -v"
                )
                if (f'"PresentValue": {data}' not in out) and (
                    f'"PendingValue": {data}' not in out
                ):
                    self.log.error("Numeric effecter value mismatch")
                    return False

        return True

    def testcase_10_set_numeric_effecter_error_case(self):
        """Test setting numeric effecter values (error case)"""
        self.log.info("Executing Test: SetNumericEffecterValueErrorCase")

        if not self.mctp_eids:
            return False

        # not available sensor ID
        na_sensor_id = 9999
        effecter_data_size = 4
        data = 50

        for mctp_id in self.mctp_eids:
            _, _, stderr = self._bmc_execute_command(
                f"pldmtool platform SetNumericEffecterValue -m {mctp_id} -i {na_sensor_id} -s {effecter_data_size} -d {data} -v"
            )
            if "Error" not in stderr:
                self.log.info("Invalid numeric effecter value response")
                return False
        return True

    def testcase_11_get_numeric_effecter(self):
        """Test reading numeric effecter value"""
        self.log.info("Executing Test: GetNumericEffecterValue")

        if not self.mctp_eids:
            return False

        expected_response_strs = [
            ['"PendingValue": ', '"PresentValue": '],
        ]
        for mctp_id in self.mctp_eids:
            for numeric_effecter_id in self.numeric_effecter_ids[mctp_id]:
                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform GetNumericEffecterValue -m {mctp_id} -i {numeric_effecter_id} -v"
                )
                if not all(x in stdout for x in expected_response_strs[0]):
                    self.log.info("Failed to get numeric effecter value")
                    return False
        return True

    def testcase_12_get_numeric_effecter_error_case(self):
        """Test reading numeric effecter value (error case)"""
        self.log.info("Executing Test: GetNumericEffecterValue")

        if not self.mctp_eids:
            return False

        # not available sensor ID
        na_sensor_id = 9999

        for mctp_id in self.mctp_eids:
            _, _, stderr = self._bmc_execute_command(
                f"pldmtool platform GetNumericEffecterValue -m {mctp_id} -i {na_sensor_id} -v"
            )
            if "Error" not in stderr:
                self.log.info("Invalid numeric effecter value response")
                return False
        self.log.info("Successfully handled error cases for numeric effecter values")
        return True

    def testcase_13_set_state_effecter_states(self):
        """Test setting state effecter states"""
        self.log.info("Executing Test: SetStateEffecterStates")

        if not self.mctp_eids:
            return False

        for mctp_id in self.mctp_eids:
            for state_effecter_id in self.state_effecter_ids[mctp_id]:
                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool platform SetStateEffecterStates -m {mctp_id} -i {state_effecter_id} -c 1 -d 1 1 -v"
                )
                if "SUCCESS" not in stdout:
                    self.log.error("Failed to set state effecter states")
                    return False

                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool raw -m {mctp_id} -v -d 0x80 0x2 0x3A {state_effecter_id} 0x0"
                )
                if (
                    f"Rx: {self.pldm_spec.get_output_header('get_state_effecter_states_success')}"
                    not in stdout
                ):
                    self.log.error("State effecter states not set as expected")
                    return False

        return True

    def testcase_14_set_state_effecter_states_error_case(self):
        """Test error handling for invalid state effecter operations"""
        self.log.info("Executing Test: SetStateEffecterStatesErrorCases")

        if not self.mctp_eids:
            return False

        na_sensor_id = 9999
        for mctp_id in self.mctp_eids:
            _, _, stderr = self._bmc_execute_command(
                f"pldmtool platform SetStateEffecterStates -m {mctp_id} -i {na_sensor_id} -c 1 -d 1 2 -v"
            )
            if "Response Message Error" not in stderr:
                self.log.error("Expected error not received for invalid state effecter")
                return False
        return True

    def testcase_15_get_state_effecter_states(self):
        """Test reading state effecter states"""
        self.log.info("Executing Test: GetStateEffecterStates")

        if not self.mctp_eids:
            return False

        for mctp_id in self.mctp_eids:
            for state_effecter_id in self.state_effecter_ids[mctp_id]:
                _, stdout, _ = self._bmc_execute_command(
                    f"pldmtool raw -m {mctp_id} -v -d 0x80 0x2 0x3A {state_effecter_id} 0x0"
                )

                if (
                    f"Rx: {self.pldm_spec.get_output_header('get_state_effecter_states_success')}"
                    not in stdout
                ):
                    self.log.error("Invalid state effecter states response")
                    return False

                self._mcp_execute_command_until(
                    command="pldm state_eff_get_state 1", key="Ok"
                )
        return True

    def testcase_16_get_state_effecter_states_error_case(self):
        """Test error handling for invalid state effecter reads"""
        self.log.info("Executing Test: GetStateEffecterStatesErrorCase")

        if not self.mctp_eids:
            return False

        na_sensor_id = 9999
        for mctp_id in self.mctp_eids:
            _, stdout, _ = self._bmc_execute_command(
                f"pldmtool raw -m {mctp_id} -v -d {self.pldm_spec.get_raw_command('get_state_effecter_states_invalid_effecter_id')}"
            )
            if (
                f"Rx: {self.pldm_spec.get_output_header('get_state_effecter_states_error')}"
                not in stdout
            ):
                self.log.error("Invalid state effecter states response")
                return False

            self._mcp_execute_command_until(
                command=f"pldm state_eff_get_state {na_sensor_id}", key="ERROR"
            )
        return True
