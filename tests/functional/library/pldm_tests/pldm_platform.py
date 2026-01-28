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


class pldm_platform(pldm_common):
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

    class PLDMEventData:
        """
        Class to hold PLDM platform event data
        """

        def __init__(self, event_id: str, event_location: str):
            self.id = event_id
            self.location = event_location

    def __init__(
        self,
        name: str = "pldm_platform",
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
        self.eventOEM = self.PLDMEventData(
            event_id="0xF1", event_location="/usr/share/pldm/eventdata/*"
        )
        self.eventCPER = self.PLDMEventData(
            event_id="0x07", event_location="/var/log/dump/*"
        )

    def setup(self):
        setup_status = super().setup(
            keep_pldm_service_active=True
        )
        if setup_status is False:
            return False
        return True

    def teardown(self):
        return super().teardown()

    def __pldm_send_pe(
        self, event: PLDMEventData, size: int, payload: str = None
    ) -> bool:
        """
        Send a PE event to the BMC.
        """
        self.log.info("Send a PE event")

        # Remove any existing event data files
        result, stdout, _ = self._bmc_execute_command(
            f"rm {event.location} 2>/dev/null || true"
        )

        cmd_str = ""
        if payload is not None:
            # If payload is provided as a string, send it as part of the PE event
            cmd_str = f"pldm send_pe {event.id} {size} {payload}"
        else:
            # If no payload is provided, send a PE event with the specified ID and size
            cmd_str = f"pldm send_pe {event.id} {size}"

        try:
            result, stdout, stderr = self._mcp_execute_command_until(
                command=cmd_str,
                key="PLDM Platform Event Completed: Success",
                timeout=60,
            )
            self.log.info(
                f"send_pe event result: {result}, stdout: {stdout}, stderr: {stderr}"
            )
        except TimeoutError as e:
            self.log.warning(f"Timeout waiting for PE event to complete: {e}")
            return False
        time.sleep(5)  # Wait for the PE event to be processed

        # Check if the event data file was created
        result, stdout, stderr = self._bmc_execute_command(
            f'hexdump -e \'"%06.6_ax " 8/1 "%.02x " "\\n"\' {event.location} ',
            max_retries=1,
        )

        if result == 0:
            self.log.info(f"Event data hexdump: {stdout}")
            if payload is None:
                # If no payload was provided, verify the generated event data
                return self.__verify_event_data_serial(stdout, size)

            # If payload is provided, verify the event data against the payload
            return self.__verify_event_data_payload(stdout, payload, size, event)
        self.log.warning("Failed to hexdump event data files")
        return False

    def __pldm_send_ppe(self, event: PLDMEventData, size: int) -> bool:
        """
        Send a PPE event to the BMC.
        """
        self.log.info("Send a PPE event")

        # Remove any existing event data files
        result, stdout, _ = self._bmc_execute_command(
            f"rm {event.location} 2>/dev/null || true"
        )

        try:
            # Send a PPE event with the specified ID and size
            result, stdout, stderr = self._mcp_execute_command_until(
                command=f"pldm send_ppe {event.id} {size}",
                key="PLDM Polled Platform Event Completed: Success",
                timeout=300,
            )
            self.log.info(
                f"send_ppe event result: {result}, stdout: {stdout}, stderr: {stderr}"
            )
        except TimeoutError as e:
            self.log.warning(f"Timeout waiting for PPE event to complete: {e}")
            return False
        time.sleep(5)  # Wait for the PPE event to be processed

        # Check if the event data file was created
        result, stdout, stderr = self._bmc_execute_command(
            f'hexdump -e \'"%06.6_ax " 8/1 "%.02x " "\\n"\' {event.location} ',
            max_retries=1,
        )

        if result == 0:
            self.log.info(f"Event data hexdump: {stdout}")
            return self.__verify_event_data_serial(stdout, size)
        self.log.warning("Failed to hexdump event data files")
        return False

    def __verify_event_data_serial(self, serial, num_bytes) -> bool:
        """
        Reads hexdump output from serial and verifies each byte value.
        :param serial: serial object (with .readline() or similar)
        :param num_bytes: number of bytes to verify
        """
        addresses = []
        payload = []
        # Build the 4-byte pattern, last two bytes are LSB num_bytes in little-endian hex
        pattern = [
            "01",
            "00",
            f"{(num_bytes & 0xFF):02x}",
            f"{((num_bytes >> 8) & 0xFF):02x}",
        ]
        self.log.info(f"Pattern is {pattern}")
        index_pattern = 0
        # Parse the serial string into address and payload arrays
        for line in serial.split("\n"):
            parts = line.strip().split()
            if len(parts) < 2:
                continue
            addresses.append(parts[0])
            payload.extend(parts[1:])

        # Check if stdout is long enough
        if len(payload) < num_bytes:
            self.log.warning(
                f"Insufficient event data: expected {num_bytes} bytes, got {len(payload)} bytes."
            )
            return False

        # Iterate over each 8-byte block
        for index in range(0, num_bytes, 8):
            start_value = index % 256
            formatted_string = f"{index:06x}"
            # Check that the address matches expected
            if formatted_string != addresses[index // 8]:
                self.log.info(
                    f"Address mismatch: {formatted_string} != {addresses[index // 8]}"
                )
                return False
            # Check each byte in the payload
            for i in range(8):
                # Prevent out-of-bounds access at end of payload
                if index + i >= len(payload):
                    break
                expected_value = f"{(start_value + i) % 256:02x}"
                # If the value doesn't match, check if it matches the special pattern
                if (expected_value != payload[index + i]) or (index_pattern != 0):
                    expected_value = pattern[index_pattern]
                    if expected_value == payload[index + i]:
                        index_pattern += 1
                        # Reset index_pattern after full pattern match
                        if index_pattern == 4:
                            index_pattern = 0
                    else:
                        self.log.info(
                            f"Payload mismatch at address {formatted_string}, "
                            f"index {index + i}: {expected_value} != {payload[index + i]}"
                        )
                        return False
        return True

    def __verify_event_data_payload(
        self, serial: str, payload: str, num_bytes, event
    ) -> bool:
        """
        Verifies that the hexdump output contains the expected payload bytes in order.
        :param serial: hexdump output as a string
        :param payload: expected payload as bytes or string
        :return: True if all bytes match, False otherwise
        """
        addresses = []
        serial_payload = []
        input_payload_raw = payload.split()
        input_payload = [x.lower().replace("0x", "") for x in input_payload_raw]

        # Parse the serial string into address and payload arrays
        for line in serial.split("\n"):
            parts = line.strip().split()
            if len(parts) < 2:
                continue
            addresses.append(parts[0])
            serial_payload.extend(parts[1:])
        # For event 0x07, skip the first 3 bytes (first byte is for size)
        if event.id == "0x07":
            input_payload = input_payload[3:]

        self.log.info(f"Serial payload: {serial_payload}")
        self.log.info(f"Input payload: {input_payload}")
        self.log.info(f"Addresses: {addresses}")
        # Iterate over each 8-byte block
        for index in range(0, num_bytes, 8):
            formatted_string = f"{index:06x}"
            # Check that the address matches expected
            if formatted_string != addresses[index // 8]:
                self.log.info(
                    f"Address mismatch: {formatted_string} != {addresses[index // 8]}"
                )
                return False
            # Check each byte in the payload
            for i in range(8):
                # Prevent out-of-bounds access at end of payload
                if index + i >= len(serial_payload):
                    break
                if input_payload[i] != serial_payload[index + i]:
                    self.log.info(
                        f"Payload mismatch at address {formatted_string}, "
                        f"index {index + i}: {input_payload[i]} != {serial_payload[index + i]}"
                    )
                    return False
        return True

    # Test case disabled due to bug: https://azurecsi.visualstudio.com/Dev/_workitems/edit/3191148,
    # this will be re-enabled once bug is fixed
    # def testcase1_pldm_send_pe_cper_small(self):
    #     """
    #     Test case: Send a platform event with CPER event ID and small payload
    #     """
    #     self.log.info("Test PLDM send PE CPER small Started")
    #     result = self.__pldm_send_pe(
    #         self.eventCPER,
    #         size=7,
    #         payload="0xAA 0xBB 0xCC 0xDD 0xEE 0xFF 0x00",
    #     )
    #     if not result:
    #         self.log.error("Test case failed")
    #         return False
    #     self.log.info("Test PLDM send PE CPER small Completed")
    #     return True

    def testcase2_pldm_send_ppe_oem_medium(self):
        """
        Test case: Send a polled platform PPE event with OEM event ID and medium payload
        """
        self.log.info("Test PLDM send PPE OEM medium Started")
        result = self.__pldm_send_ppe(self.eventOEM, size=1200)
        if not result:
            self.log.error("Test case failed")
            return False
        self.log.info("Test PLDM send PPE OEM medium Completed")
        return True

    def testcase3_pldm_send_ppe_cper_medium(self):
        """
        Test case: Send a polled platform PPE event with CPER event ID and medium payload
        """
        self.log.info("Test PLDM send PPE CPER medium Started")
        result = self.__pldm_send_ppe(self.eventCPER, size=1300)
        if not result:
            self.log.error("Test case failed")
            return False
        self.log.info("Test PLDM send PPE CPER medium Completed")
        return True

    def testcase4_pldm_send_ppe_oem_large(self):
        """
        Test case: Send a polled platform PPE event with OEM event ID and large payload
        """
        self.log.info("Test PLDM send PPE OEM large Started")
        result = self.__pldm_send_ppe(self.eventOEM, size=80000)
        if not result:
            self.log.error("Test case failed")
            return False
        self.log.info("Test PLDM send PPE OEM large Completed")
        return True

    def testcase5_pldm_send_ppe_cper_large(self):
        """
        Test case: Send a polled platform PPE event with CPER event ID and large payload
        """
        self.log.info("Test PLDM send PPE CPER large Started")
        result = self.__pldm_send_ppe(self.eventCPER, size=90000)
        if not result:
            self.log.error("Test case failed")
            return False
        self.log.info("Test PLDM send PPE CPER large Completed")
        return True
