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
from library.pldm_tests.pldm_spec_parser import pldm_spec_parser
from pythia.tdk.echofalls.constants.dut_types import DeviceType
from pythia.tdk.echofalls.echofalls_base_test import EchoFallsBaseTest
from pathlib import Path
from RscmHelperLibrary import RscmHelperLibrary


class pldm_base(EchoFallsBaseTest):
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
        name: str = "pdlm_base_test",
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
        self.mctp_eids = []

    def setup(self):
        self.dut.setup()
        if self.dut.get_dut_type() == DeviceType.BIGFPGA:
            self.log.warning(
                "Device type is bigFPGA. Performing an additional OOB reset ..."
            )
            KngPythiaTestSetup.fpga_oob_reset(self.log)
        elif self.dut.get_dut_type() == DeviceType.RVP:
            self.log.warning("Device type is RVP. Performing SoC Reset ...")
            cred_path = os.environ.get("SECURE_FILE_PATH")
            creds = KngPythiaTestSetup.load_credentials_from_yaml(cred_path)
            self.rscm_helper = RscmHelperLibrary(
                rm_host=self.host_config.rack_scm.host,
                bmc_host=self.dut.mb.node_0.dcscm.bmc.ip,
                rm_user=creds["RM_USER"],
                rm_password=creds["RM_PASSWORD"],
                bmc_user=creds["BMC_USER"],
                bmc_password=creds["BMC_PASSWORD"],
                node=self.host_config.node_id,
            )
            self.rscm_helper.rscm_soc_reset()

        self.bmc_cli = self.dut.mb.node_0.dcscm.bmc.cli
        self.bmc_cli.open()
        assert self.bmc_cli.is_open()

        if self.dut.get_dut_type() == DeviceType.RVP:
            self.rscm_helper.set_bmc_uart_mux_mcp(self.bmc_cli)

        self.core_mcp_channel = (
            self.dut.mb.node_0.soc.primary_die.mcp.channel_manager.get_current_channel()
        )
        self.core_mcp_channel.open()
        assert self.core_mcp_channel.is_open()

        self.pldm_spec = pldm_spec_parser()
        self.pldm_spec.load_spec_data("pldm_spec_data.json")

        delay_10_minutes = 10 * 60
        self.core_mcp_channel.read_until(
            key="StartupShutdownSvc::LocalCoreSyncStageRemoteEnd",
            timeout_seconds=delay_10_minutes,
        )

    def teardown(self):
        if self.dut.get_dut_type() == DeviceType.RVP:
            self.rscm_helper.set_bmc_uart_mux_scp(self.bmc_cli)
        self.dut.teardown()
        time.sleep(10)

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

    def __bmc_execute_command(self, command):
        cmd_str = command
        result, stdout, stderr = self.bmc_cli.execute_command(
            command=cmd_str, command_args=[], timeout_secs=1
        )
        if result != 0:
            self.log.info(f"~$ failed to execute command: {cmd_str}")
        return result, stdout, stderr

    def testcase1_pldm_get_mctp_id(self):
        """
        Get MCP EID from the BMC
        """
        self.log.info("!Executing Test Function: testcase1_pldm_get_mctp_id")

        result, stdout, _ = self.__bmc_execute_command(command="mctptool list")
        if result != 0:
            return False
        lines = stdout.strip().split("\n")

        # Example output of mctptool list:
        # 12  | Pa-RoT Cerberus on BMC TIP | I2C Bus: 0 Addr: 0x82 | MctpControl SPDM VDPCI
        # 16  | HSP                  | I2C Bus: 9 Addr: 0xaa | MctpControl SPDM VDPCI
        # 17  | MCP                  | I3C Bus: 1 Addr: 0x596434f0200 | MctpControl PLDM
        # 64  | Cerberus on LION     | I2C Bus: 16 Addr: 0x82 | MctpControl SPDM VDPCI
        for line in lines:
            parts = line.split("|")
            if len(parts) > 1 and "MCP" in parts[1]:
                mcp_id = int(parts[0].strip())
                self.mctp_eids.append(mcp_id)
                break
        if self.mctp_eids == []:
            self.log.info("MCP ID not found")
            return False
        self.log.info(f"Found MCTP EIDs: {self.mctp_eids}")
        return True

    def testcase2_get_pldm_tid(self):
        """
        Test getting the Terminus ID using PLDM
        """
        self.log.info("!Executing Test Function: testcase2_get_pldm_tid")
        if len(self.mctp_eids) == 0:
            return False

        for mctp_id in self.mctp_eids:
            result, stdout, _ = self.__bmc_execute_command(
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

            self.log.info(f"MCP ID: {mctp_id}")

            result, stdout, _ = self.__bmc_execute_command(
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

            result, stdout, _ = self.__bmc_execute_command(
                command=f"pldmtool base SetTID -m {mctp_id} -t {tid_test_expected} -v"
            )
            if result != 0:
                return False

            if self.pldm_spec.get_output_header("set_tid") not in stdout:
                self.log.info("SetTID response header not found in output")
                return False

            result, stdout, _ = self.__bmc_execute_command(
                command=f"pldmtool base GetTID -m {mctp_id} -v"
            )

            rx_byte_str = self.__get_rx_byte_string_from_output(stdout)
            tid_test_actual = int(
                self.__get_nth_last_byte_from_byte_string(
                    rx_byte_str, self.pldm_spec.get_offset("tid_data_offset")
                )
            )
            self.log.info(f"TID is {tid_test_actual}")

            result, stdout, _ = self.__bmc_execute_command(
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
            result, stdout, _ = self.__bmc_execute_command(
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
                result, stdout, _ = self.__bmc_execute_command(
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
            result, stdout, stderr = self.__bmc_execute_command(
                command=f"pldmtool base GetPLDMVersion -m {mctp_id} -t {non_existing_pldm_id} -v"
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
                result, stdout, _ = self.__bmc_execute_command(
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
                result, stdout, stderr = self.__bmc_execute_command(
                    command=f"pldmtool base GetPldmCommands -m {mctp_id} -t {non_existing_pldm_id} --version {pldm_type['version_encoded']} -v",
                )
                if "FAILED" not in stderr:
                    self.log.info("Expected error not found when PLDM type is wrong")
                    return False

                result, stdout, stderr = self.__bmc_execute_command(
                    command=f"pldmtool base GetPldmCommands -m {mctp_id} -t {pldm_type['id']} --version {non_existing_pldm_version} -v"
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
            result, stdout, _ = self.__bmc_execute_command(
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
            result, stdout, _ = self.__bmc_execute_command(
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
                result, stdout, _ = self.__bmc_execute_command(
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
            result, stdout, stderr = self.__bmc_execute_command(
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
            result, stdout, _ = self.__bmc_execute_command(
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
