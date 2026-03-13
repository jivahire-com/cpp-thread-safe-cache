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
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
from pathlib import Path
from RscmHelperLibrary import RscmHelperLibrary


class pldm_base(pldm_common):
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
        name: str = "pldm_base_test",
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

    def setup(self):
        """Perform an initial reset and boot so subsequent PLDM test suites can
        start from a clean system state without repeating these steps.
        """
        return super().setup(perform_reset_boot=True)

    def teardown(self):
        return super().teardown()

    def __get_nth_last_byte_from_byte_string(self, data, n):
        # strip removes leading and trailing whitespaces
        # while split creates a list of bytes strings, considering whitespaces as delimiter
        bytes_list = data.strip().split()
        if len(bytes_list) == 0 or n > len(bytes_list):
            return None
        return bytes_list[-1 * n]

    def __get_nth_start_byte_from_byte_string(self, data, n):
        # strip removes leading and trailing whitespaces
        # while split creates a list of bytes strings, considering whitespaces as delimiter
        bytes_list = data.strip().split()
        if len(bytes_list) == 0 or n > len(bytes_list):
            return None
        return bytes_list[(n - 1)]

    def __get_rx_byte_string_from_output(self, output):
        match = re.search(r"pldmtool: Rx:\s*([0-9A-Fa-f\s]+)", output)
        if match is None:
            return []
        return match.group(1)

    def testcase1_pldm_get_mctp_id(self):
        """
        Test that at least one MCTP endpoint ID is discovered.

        Returns:
            bool: True if one or more MCTP endpoint IDs are present in mctp_eids, False otherwise.
        """
        return len(self.mctp_eids) != 0

    def testcase2_get_pldm_tid(self):
        """
        Test getting the Terminus ID using PLDM
        """
        self.log.info("!Executing Test Function: testcase2_get_pldm_tid")
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:
            result, stdout, _ = self._bmc_execute_command(
                command=f"pldmtool base GetTID -m {mctp_id} -v"
            )
            if result != 0:
                return False

            if self.pldm_spec.get_output_header("get_tid") not in stdout:
                self.log.info("GetTID response header was not found in output")
                return False

            rx_byte_str = self.__get_rx_byte_string_from_output(stdout)
            tid_actual = int(
                self.__get_nth_last_byte_from_byte_string(
                    rx_byte_str, self.pldm_spec.get_offset("tid_data_offset")
                )
            )
            self.log.info(f"TID is {tid_actual}")

        return True

    def testcase2_set_pldm_tid(self):
        """
        Test setting the Terminus ID using PLDM
        """
        self.log.info("!Executing Test Function: testcase2_set_pldm_tid")
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:

            tid_previous = None
            tid_test_actual = None

            self.log.info(f"MCTP ID: {mctp_id}")

            result, stdout, _ = self._bmc_execute_command(
                command=f"pldmtool base GetTID -m {mctp_id} -v"
            )
            if result != 0:
                return False

            rx_byte_str = self.__get_rx_byte_string_from_output(stdout)
            tid_previous = int(
                self.__get_nth_last_byte_from_byte_string(
                    rx_byte_str, self.pldm_spec.get_offset("tid_data_offset")
                )
            )
            self.log.info(f"TID is {tid_previous}")
            tid_test_expected = tid_previous + 1

            result, stdout, _ = self._bmc_execute_command(
                command=f"pldmtool base SetTID -m {mctp_id} -t {tid_test_expected} -v"
            )
            if result != 0:
                return False

            if self.pldm_spec.get_output_header("set_tid") not in stdout:
                self.log.info("SetTID response header not found in output")
                return False

            result, stdout, _ = self._bmc_execute_command(
                command=f"pldmtool base GetTID -m {mctp_id} -v"
            )

            rx_byte_str = self.__get_rx_byte_string_from_output(stdout)
            tid_test_actual = int(
                self.__get_nth_last_byte_from_byte_string(
                    rx_byte_str, self.pldm_spec.get_offset("tid_data_offset")
                )
            )
            self.log.info(f"TID is {tid_test_actual}")

            result, stdout, _ = self._bmc_execute_command(
                command=f"pldmtool base SetTID -m {mctp_id} -t {tid_previous} -v"
            )

            rx_byte_str = self.__get_rx_byte_string_from_output(stdout)
            tid_test_previous_2 = int(
                self.__get_nth_last_byte_from_byte_string(
                    rx_byte_str, self.pldm_spec.get_offset("tid_data_offset")
                )
            )
            self.log.info(f"TID is {tid_test_previous_2}")

            if tid_test_actual != tid_test_expected:
                return False

        return True

    def testcase3_verify_pldm_types(self):
        """
        Test getting the PLDM types
        """
        self.log.info("!Executing Test Function: testcase3_verify_pldm_types")
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:
            result, stdout, _ = self._bmc_execute_command(
                command=f"pldmtool base GetPLDMTypes -m {mctp_id} -v"
            )
            if result != 0:
                return False

            if self.pldm_spec.get_output_header("get_pldm_types") not in stdout:
                self.log.info("GetPLDMTypes response header was not found in output")
                return False

            rx_byte_str = self.__get_rx_byte_string_from_output(stdout)
            actual_pldm_type_byte_code = int(
                self.__get_nth_last_byte_from_byte_string(
                    rx_byte_str, self.pldm_spec.get_offset("pldm_type_offset")
                )
            )

            test_output = True
            for _, expected_pldm_type in enumerate(self.pldm_spec.get_all_pldm_types()):
                expected_pldm_type_code = expected_pldm_type["id"]
                if ((2**expected_pldm_type_code) & actual_pldm_type_byte_code) == 0b0:
                    self.log.info(
                        f"Expected PLDM Type {expected_pldm_type_code} was not found in GetPLDMType response"
                    )
                    test_output = False

        return test_output

    def testcase4_verify_pldm_versions(self):
        """
        Test getting the PLDM type versions
        """
        self.log.info("!Executing Test Function: testcase4_verify_pldm_versions")
        if len(self.mctp_eids) == 0:
            return False

        test_output = True
        for mctp_id in self.mctp_eids:
            for index, pldm_type in enumerate(self.pldm_spec.get_all_pldm_types()):
                pldm_type_id = pldm_type["id"]
                result, stdout, _ = self._bmc_execute_command(
                    command=f"pldmtool base GetPLDMVersion -m {mctp_id} -t {pldm_type_id} -v"
                )
                if result != 0:
                    return False

                if self.pldm_spec.get_output_header("get_pldm_version") not in stdout:
                    self.log.info(
                        "GetPLDMVersion response header was not found in output"
                    )
                    test_output = False

                expected_output = pldm_type["version"]
                if expected_output not in stdout:
                    self.log.info(
                        f"GetPLDMVersion response version does not match with expected "
                        f"version {expected_output} for a given type id {pldm_type_id}"
                    )
                    test_output = False

        return test_output

    def testcase4_verify_pldm_version_error(self):
        """
        Test PLDM type versions error code
        """
        self.log.info("!Executing Test Function: testcase4_verify_pldm_version_error")
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:
            non_existing_pldm_id = 5000
            result, stdout, stderr = self._bmc_execute_command(
                command=f"pldmtool base GetPLDMVersion -m {mctp_id} -t {non_existing_pldm_id} -v",
                max_retries=1,
            )
            if "FAILED" not in stderr:
                return False
        return True

    def testcase5_verify_pldm_commands(self):
        """
        Gets the PLDM Type Supported Commands
        """
        self.log.info("!Executing Test Function: testcase5_verify_pldm_commands")
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:
            test_output = True
            for _, pldm_type in enumerate(self.pldm_spec.get_all_pldm_types()):
                self.log.info(f"PLDM Type {pldm_type}")
                result, stdout, _ = self._bmc_execute_command(
                    command=f"pldmtool base GetPldmCommands -m {mctp_id} -t {pldm_type['id']} --version {pldm_type['version_encoded']} -v",
                )
                if result != 0:
                    test_output = False
                    continue
                if self.pldm_spec.get_output_header("get_pldm_commands") not in stdout:
                    self.log.info(
                        "GetPLDMCommands response header was not found in output"
                    )
                    test_output = False
                    continue

                expected_commands_list = pldm_type["commands"]
                for expected_command in expected_commands_list:
                    if expected_command not in stdout:
                        self.log.info(
                            f"Expected command {expected_command} not found for PLDM Type {pldm_type['id']}"
                        )
                        test_output = False

        return test_output

    def testcase5_verify_pldm_commands_error(self):
        """
        Gets the PLDM Type Supported Commands - error case when PLDM type or PLDM type version is wrong
        """
        self.log.info("!Executing Test Function: testcase5_verify_pldm_commands_error")
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:
            non_existing_pldm_id = 5000
            non_existing_pldm_version = "0x00 0x00 0x00 0x00"

            for _, pldm_type in enumerate(self.pldm_spec.get_all_pldm_types()):
                result, stdout, stderr = self._bmc_execute_command(
                    command=f"pldmtool base GetPldmCommands -m {mctp_id} -t {non_existing_pldm_id} --version {pldm_type['version_encoded']} -v",
                    max_retries=1,
                )
                if "FAILED" not in stderr:
                    self.log.info("Expected error not found when PLDM type is wrong")
                    return False

                result, stdout, stderr = self._bmc_execute_command(
                    command=f"pldmtool base GetPldmCommands -m {mctp_id} -t {pldm_type['id']} --version {non_existing_pldm_version} -v",
                    max_retries=1,
                )
                rx_byte_str = self.__get_rx_byte_string_from_output(stdout)
                actual_pldm_commands_version_error = (
                    self.__get_nth_last_byte_from_byte_string(
                        rx_byte_str,
                        self.pldm_spec.get_offset("pldm_commands_version_error_offset"),
                    )
                )
                if "Response Message Error" not in stderr or (
                    actual_pldm_commands_version_error
                    != self.pldm_spec.get_error_code("pldm_commands_version_error")
                ):
                    self.log.info("Expected error not found when PLDM version is wrong")
                    return False

        return True

    def testcase6_verify_get_pdr_repository_info(self):
        """
        Gets the PLDM Terminus PDR Information
        """
        self.log.info(
            "!Executing Test Function: testcase6_verify_get_pdr_repository_info"
        )
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:
            result, stdout, _ = self._bmc_execute_command(
                command=f"pldmtool platform GetPDRRepositoryInfo -m {mctp_id} -v",
            )

            if result != 0:
                return False

            if (
                self.pldm_spec.get_output_header("get_pdr_repositories_info")
                not in stdout
            ):
                self.log.info(
                    "GetPDRRepositoriesInfo response header was not found in output"
                )
                return False

        return True

    def testcase6_verify_get_pdr(self):
        """
        Gets the PLDM PDR
        """
        self.log.info("!Executing Test Function: testcase6_verify_get_pdr")
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:
            result, stdout, _ = self._bmc_execute_command(
                command=f"pldmtool platform GetPDRRepositoryInfo -m {mctp_id} -v",
            )
            if result != 0:
                return False

            if (
                self.pldm_spec.get_output_header("get_pdr_repositories_info")
                not in stdout
            ):
                self.log.info(
                    "GetPDRRepositoriesInfo response header was not found in output"
                )
                return False

            pdr_count_list = re.findall(r'"RecordCount": (\d+),', stdout)
            pdr_handle = 0

            if pdr_count_list == []:
                self.log.info("RecordCount was not found in output")
                return False

            pdr_count = int(pdr_count_list[0])

            for _ in range(pdr_count):
                result, stdout, _ = self._bmc_execute_command(
                    command=f"pldmtool platform GetPDR -m {mctp_id} -d {pdr_handle} -v",
                )
                if (
                    result != 0
                    or self.pldm_spec.get_output_header("get_pdr") not in stdout
                ):
                    self.log.info("GetPDR response header was not found in output")
                    return False

                pdr_handle_match = re.search(r'"nextRecordHandle": (\d+),', stdout)
                pdr_type_match = re.search(r'"PDRType": "([^"]*)",', stdout)

                if pdr_handle_match:
                    pdr_handle = int(pdr_handle_match.group(1))
                else:
                    self.log.info("Failed to extract nextRecordHandle")
                    return False

                actual_pdr_type = pdr_type_match.group(1) if pdr_type_match else None
                found_flag = False
                for expected_pdr_type in self.pldm_spec.get_pdr_types():
                    if actual_pdr_type == expected_pdr_type:
                        found_flag = True
                        break
                if found_flag is False:
                    self.log.info(f"For PDR Handle: {pdr_handle}")
                    self.log.info("PDRType was not found in expected_pdr_types list")
                    return False

        return True

    def testcase6_verify_get_pdr_error(self):
        """
        Gets the PLDM PDR Error
        """
        self.log.info("!Executing Test Function: testcase6_verify_get_pdr_error")
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:
            not_supported_pldm_id = 0
            result, stdout, stderr = self._bmc_execute_command(
                command=f"pldmtool platform GetPDR -m {mctp_id} -v -t {not_supported_pldm_id}",
            )
            if result != 0:
                return False
            if self.pldm_spec.get_output_header("get_pdr") not in stdout:
                self.log.info("GetPDR response header was not found in output")
                return False
            if (
                f"PDR type '{not_supported_pldm_id}' is not supported or invalid"
                not in stderr
            ):
                self.log.info(
                    "GetPDR command: error not expected when PLDM type is wrong"
                )
                return False

            non_existing_pldm_id = 5000
            result, stdout, _ = self._bmc_execute_command(
                command=f"pldmtool platform GetPDR -m {mctp_id} -d {non_existing_pldm_id} -v",
            )
            if result != 0:
                return False
            if self.pldm_spec.get_output_header("get_pdr") not in stdout:
                self.log.info("GetPDR response header was not found in output")
                return False
            rx_byte_str = self.__get_rx_byte_string_from_output(stdout)
            actual_pldm_invalid_record_error = (
                self.__get_nth_start_byte_from_byte_string(
                    rx_byte_str, self.pldm_spec.get_offset("pldm_error_offset")
                )
            )
            if actual_pldm_invalid_record_error != self.pldm_spec.get_error_code(
                "pldm_invalid_record_error"
            ):
                self.log.info(
                    "GetPDR command: error not expected when PLDM type is wrong"
                )
                return False

        return True
