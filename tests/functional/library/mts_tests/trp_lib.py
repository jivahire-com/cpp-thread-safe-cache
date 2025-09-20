# Copyright (c) Microsoft Corporation. All rights reserved.

# to  run from repo environment
# & (Join-Path ([System.Environment]::GetEnvironmentVariable("REPO_APP_PATH_python.win64", "Process")) "/tools/python.exe") tests/functional/pythia/pylibs/mts_tests/trp_lib.py
import os
import sys
import time
import ctypes
import logging

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
# Add the icc_mhu_tests directory to access icc_mhu_ap_lib
icc_mhu_tests_dir = os.path.join(pylibs_dir, "icc_mhu_tests")
sys.path.extend(
    [
        pylibs_dir,
        current_dir,
        icc_mhu_tests_dir,
    ]
)

logger = logging.getLogger(__name__)

from fpfw_automation_primitives.serial.telnet import (
    Telnet_,
)

from dcp_protocol import (
    data_collection_protocol,
    struct_to_hex_string,
    byte_list_to_hex_string,
)
from trp_protocol import transfer_relay_protocol

# Import ICC MHU AP library
from icc_mhu_ap_lib import IccMhuAp, CoreMemoryMapInterface, IccMhu

from typing import Optional
from abc import ABC, abstractmethod
from enum import Enum


class CoreMemoryMapAccess(ABC):
    """Interface for endpoints that support direct memory file operations"""

    @abstractmethod
    def read_to_file(self, address: int, size: int, filename: str) -> None:
        """
        Read memory to file.

        Args:
            address: Memory address to start reading from
            size: Number of bytes to read
            filename: Output file path

        Raises:
            RuntimeError: If memory read operation fails
            IOError: If file write operation fails
            ValueError: If invalid parameters provided
        """
        pass


# Abstract Base Class (ABC)
class trp_endpoint(ABC):

    def __init__(
        self, source_die: ctypes.c_uint8, source_cpu: transfer_relay_protocol.cpu_type
    ):
        self.source_die = source_die  # Store common attribute
        self.source_cpu = source_cpu
        self.sequence_number = 0
        self.default_timeout_sec = 60

    @abstractmethod
    def send_dcp_message(
        self,
        dest_die: ctypes.c_uint8,
        dest_cpu: transfer_relay_protocol.cpu_type,
        client_id: data_collection_protocol.mts_client_id_t,
        dcp_msg: list[int],
        timeout_sec: Optional[int] = None,
    ) -> bytearray:
        pass

    @abstractmethod
    def send_trp_message(self) -> str:
        pass

    def get_next_sequence_number(self) -> int:
        """Get next sequence number and increment counter"""
        self.sequence_number = (self.sequence_number + 1) & 0x1FF
        return self.sequence_number

    def create_dcp_forward_trp_header(
        self,
        dest_die: ctypes.c_uint8,
        dest_cpu: transfer_relay_protocol.cpu_type,
        client_id: data_collection_protocol.mts_client_id_t,
        payload_size: int,
    ) -> transfer_relay_protocol.trp_msg_hdr_t:
        """
        Create a TRP message header for DCP forwarding.

        Args:
            dest_die: Destination die ID
            dest_cpu: Destination CPU type
            client_id: MTS client ID
            payload_size: Size of the DCP message payload

        Returns:
            Populated TRP message header for DCP forwarding
        """
        trp_msg_hdr = transfer_relay_protocol.trp_msg_hdr_t()
        trp_msg_hdr.src_node.die_id = self.source_die
        trp_msg_hdr.src_node.core_id = self.source_cpu.value
        trp_msg_hdr.dest_node.die_id = dest_die
        trp_msg_hdr.dest_node.core_id = dest_cpu.value
        trp_msg_hdr.mts_client_id = client_id.value
        trp_msg_hdr.trp_msg_id = (
            transfer_relay_protocol.trp_msg_id_t.TRP_MSG_ID_DCP_FORWARD.value
        )
        trp_msg_hdr.source_seq_num = self.get_next_sequence_number()
        trp_msg_hdr.payload_size = payload_size
        return trp_msg_hdr

    def log_trp_response_headers(
        self,
        trp_msg_hdr: transfer_relay_protocol.trp_msg_hdr_t,
        response_bytes: bytearray,
    ) -> None:
        """
        Log TRP and DCP response headers for debugging.

        Args:
            trp_msg_hdr: The original TRP message header that was sent
            response_bytes: The response bytes containing TRP header + DCP response
        """

        # Helper function to get CPU type name
        def get_cpu_name(core_id: int) -> str:
            try:
                cpu_type = transfer_relay_protocol.cpu_type(core_id)
                return cpu_type.name
            except ValueError:
                return f"UNKNOWN_CPU_{core_id}"

        # Helper function to get TRP status name
        def get_trp_status_name(status_value: int) -> str:
            try:
                trp_status = transfer_relay_protocol.trp_status_t(status_value)
                return f"{trp_status.name} ({status_value:x})"
            except ValueError:
                return f"UNKNOWN_TRP_STATUS_{status_value:x}"

        # Helper function to get DCP status name
        def get_dcp_status_name(status_value: int) -> str:
            try:
                dcp_status = data_collection_protocol.dcp_status_t(status_value)
                return f"{dcp_status.name} ({status_value:x})"
            except ValueError:
                return f"UNKNOWN_DCP_STATUS_{status_value:x}"

        # Helper function to get MTS client ID name
        def get_mts_client_id_name(client_id_value: int) -> str:
            try:
                mts_client_id = data_collection_protocol.mts_client_id_t(
                    client_id_value
                )
                return f"{mts_client_id.name} ({client_id_value})"
            except ValueError:
                return f"UNKNOWN_MTS_CLIENT_ID_{client_id_value}"

        # Helper function to get TRP message ID name
        def get_trp_msg_id_name(msg_id_value: int) -> str:
            try:
                trp_msg_id = transfer_relay_protocol.trp_msg_id_t(msg_id_value)
                return f"{trp_msg_id.name} ({msg_id_value:x})"
            except ValueError:
                return f"UNKNOWN_TRP_MSG_ID_{msg_id_value:x}"

        # Helper function to get DCP message ID name
        def get_dcp_msg_id_name(msg_id_value: int) -> str:
            try:
                dcp_msg_id = data_collection_protocol.dcp_msg_id_t(msg_id_value)
                return f"{dcp_msg_id.name} ({msg_id_value:x})"
            except ValueError:
                return f"UNKNOWN_DCP_MSG_ID_{msg_id_value:x}"

        # Log the TRP header fields
        logger.debug(f"Response TRP Header")
        logger.debug(f"TRP Header - Source Die ID: {trp_msg_hdr.src_node.die_id}")
        logger.debug(
            f"TRP Header - Source Core ID: {trp_msg_hdr.src_node.core_id} "
            f"({get_cpu_name(trp_msg_hdr.src_node.core_id)})"
        )
        logger.debug(f"TRP Header - Destination Die ID: {trp_msg_hdr.dest_node.die_id}")
        logger.debug(
            f"TRP Header - Destination Core ID: {trp_msg_hdr.dest_node.core_id} "
            f"({get_cpu_name(trp_msg_hdr.dest_node.core_id)})"
        )
        logger.debug(
            f"TRP Header - Message status: {get_trp_status_name(trp_msg_hdr.trp_msg_status)}"
        )
        logger.debug(
            f"TRP Header - MTS Client ID: {get_mts_client_id_name(trp_msg_hdr.mts_client_id)}"
        )
        logger.debug(
            f"TRP Header - TRP Message ID: {get_trp_msg_id_name(trp_msg_hdr.trp_msg_id)}"
        )
        logger.debug(
            f"TRP Header - Source Sequence Number: 0x{trp_msg_hdr.source_seq_num:X}"
        )
        logger.debug(f"TRP Header - Payload Size: 0x{trp_msg_hdr.payload_size:X}")

        # Log the DCP header fields if response is large enough
        if len(response_bytes) >= ctypes.sizeof(
            transfer_relay_protocol.trp_msg_hdr_t
        ) + ctypes.sizeof(data_collection_protocol.dcp_msg_hdr_t):
            dcp_msg_bytes = response_bytes[
                ctypes.sizeof(transfer_relay_protocol.trp_msg_hdr_t) :
            ]
            dcp_msg_hdr = data_collection_protocol.dcp_msg_hdr_t.from_buffer_copy(
                dcp_msg_bytes
            )
            logger.debug(f"Response DCP Header")
            logger.debug(
                f"DCP Header - Client ID: {get_mts_client_id_name(dcp_msg_hdr.client_id)}"
            )
            logger.debug(
                f"DCP Header - Message ID: {get_dcp_msg_id_name(dcp_msg_hdr.msg_id)}"
            )
            logger.debug(f"DCP Header - Sequence Number: 0x{dcp_msg_hdr.seq_num:X}")
            logger.debug(
                f"DCP Header - Message Status: {get_dcp_status_name(dcp_msg_hdr.msg_status)}"
            )
            logger.debug(f"DCP Header - Payload Size: 0x{dcp_msg_hdr.payload_size:X}")
        else:
            logger.debug(
                f"Response too short to contain DCP header: {len(response_bytes)} bytes"
            )


class mts_cli_trp_endpoint(trp_endpoint):
    def __init__(
        self,
        source_comm_channel,
        source_die: ctypes.c_uint8,
        source_cpu: transfer_relay_protocol.cpu_type,
    ):
        super().__init__(source_die, source_cpu)
        self.source_comm_channel = source_comm_channel

    def send_trp_message(self) -> str:
        self.source_comm_channel.write(write_string="?\n")
        response = self.source_comm_channel.read_until(key="hm", timeout_seconds=2)
        return response

    def send_dcp_message(
        self,
        dest_die: ctypes.c_uint8,
        dest_cpu: transfer_relay_protocol.cpu_type,
        client_id: data_collection_protocol.mts_client_id_t,
        dcp_msg: list[int],
        timeout_sec: Optional[int] = None,
    ) -> bytearray:

        trp_msg_hdr = self.create_dcp_forward_trp_header(
            dest_die, dest_cpu, client_id, len(dcp_msg)
        )

        send_str = (
            "mts trp_send "
            + struct_to_hex_string(trp_msg_hdr)
            + byte_list_to_hex_string(dcp_msg)
        )

        logger.debug(f"\nSending DCP in TRP message: {send_str}\n")
        self.source_comm_channel.write_line(write_string=send_str)
        try:
            cmd_timeout_sec = (
                timeout_sec if timeout_sec is not None else self.default_timeout_sec
            )
            logger.debug(f"Waiting for TRP response for {cmd_timeout_sec} seconds")
            response = self.source_comm_channel.read_until(
                key="TrpRx", timeout_seconds=cmd_timeout_sec
            )
        except Exception as e:
            logger.error(f"Error while reading response: {e}")
            response = ""  # or handle as appropriate

        rsp_index = response.find("Rsp: ")
        if rsp_index == -1:
            raise ValueError(f"TRP Response not found in {response}")

        response_str = response[rsp_index + 4 :].strip()
        response_str = response_str.split("\n")[
            0
        ]  # Take only the part before the newline

        # Split the string into individual components and convert to list of integers
        response_list = [int(x) for x in response_str.split()]

        # Convert response_list to byte array
        response_bytes = bytearray(response_list)
        logger.debug(
            f"\nDCP in TRP response: {' '.join(f'{b:02X}' for b in response_bytes)}\n"
        )

        # Log response headers using common method
        self.log_trp_response_headers(trp_msg_hdr, response_bytes)

        if len(response_bytes) < (
            ctypes.sizeof(transfer_relay_protocol.trp_msg_hdr_t)
            + ctypes.sizeof(data_collection_protocol.dcp_msg_hdr_t)
        ):
            raise ValueError(
                "Response is too short to contain trp_msg_hdr_t and dcp_msg_hdr_t, {response_bytes}"
            )

        # Extract DCP response (skip TRP header)
        dcp_msg_bytes = response_bytes[
            ctypes.sizeof(transfer_relay_protocol.trp_msg_hdr_t):
        ]
        return dcp_msg_bytes


class mts_icc_mhu_trp_endpoint(trp_endpoint, CoreMemoryMapAccess):
    """TRP endpoint implementation for ICC MHU interface"""

    def __init__(
        self,
        icc_mhu: IccMhu,
        source_die: ctypes.c_uint8,
        source_cpu: transfer_relay_protocol.cpu_type,
    ):
        super().__init__(source_die, source_cpu)
        self.icc_mhu = icc_mhu

    def send_trp_message(self) -> str:
        """Send a simple TRP message via ICC MHU interface"""
        logger.debug("Sending TRP message via ICC MHU interface")

        try:
            # Send a simple status query command
            status_packet = bytearray([0x53, 0x54, 0x41, 0x54])
            self.icc_mhu.send_packet(status_packet, timeout_sec=5)

            # Get response
            response = self.icc_mhu.get_packet(timeout_sec=5)
            if response:
                return f"TRP message sent via ICC MHU, response: {' '.join(f'{b:02X}' for b in response)}"
            else:
                return "TRP message sent via ICC MHU, no response received"

        except Exception as e:
            logger.error(f"Failed to send TRP message: {e}")
            return f"TRP message failed: {e}"

    def send_dcp_message(
        self,
        dest_die: ctypes.c_uint8,
        dest_cpu: transfer_relay_protocol.cpu_type,
        client_id: data_collection_protocol.mts_client_id_t,
        dcp_msg: list[int],
        timeout_sec: Optional[int] = None,
    ) -> bytearray:
        """Send a DCP message wrapped in TRP via ICC MHU interface"""

        # Create TRP message header using common method
        trp_msg_hdr = self.create_dcp_forward_trp_header(
            dest_die, dest_cpu, client_id, len(dcp_msg)
        )

        # Convert TRP header to bytes and combine with DCP payload
        trp_header_bytes = bytearray(trp_msg_hdr)
        packet = trp_header_bytes + bytearray(dcp_msg)

        logger.debug("Sending DCP in TRP message via ICC MHU:")
        logger.debug(f"TRP Header: {' '.join(f'{b:02X}' for b in trp_header_bytes)}")
        logger.debug(f"DCP Payload: {' '.join(f'{b:02X}' for b in dcp_msg)}")

        try:
            cmd_timeout_sec = (
                timeout_sec if timeout_sec is not None else self.default_timeout_sec
            )

            # Use ICC MHU send_packet with combined TRP+DCP packet
            self.icc_mhu.send_packet(packet, timeout_sec=cmd_timeout_sec)

            # Wait for response using ICC MHU get_packet
            response_bytes = self.icc_mhu.get_packet(timeout_sec=cmd_timeout_sec)

            if response_bytes:
                logger.debug(
                    f"DCP in TRP response received: {' '.join(f'{b:02X}' for b in response_bytes)}"
                )

                # Log response headers using common method
                self.log_trp_response_headers(trp_msg_hdr, response_bytes)

                # The response should contain TRP header + DCP response
                # Extract DCP response (skip TRP header)
                trp_header_size = ctypes.sizeof(transfer_relay_protocol.trp_msg_hdr_t)
                if len(response_bytes) > trp_header_size:
                    dcp_response = response_bytes[trp_header_size:]
                    return dcp_response
                else:
                    raise ValueError(f"Response too short: {len(response_bytes)} bytes")
            else:
                raise TimeoutError(
                    f"No response received within {cmd_timeout_sec} seconds"
                )

        except Exception as e:
            logger.error(f"Error while sending DCP message via ICC MHU: {e}")
            raise RuntimeError(f"Failed to send DCP message via ICC MHU: {e}")

    def read_to_file(self, address: int, size: int, filename: str) -> None:
        """
        Read memory to file using the core memory interface.

        Args:
            address: Memory address to start reading from
            size: Number of bytes to read
            filename: Output file path

        Raises:
            RuntimeError: If memory read operation fails
            IOError: If file write operation fails
            ValueError: If invalid parameters provided
        """
        if size <= 0:
            raise ValueError(f"Invalid size: {size}. Size must be positive.")

        if address < 0:
            raise ValueError(
                f"Invalid address: 0x{address:X}. Address must be non-negative."
            )

        try:
            logger.info(
                f"Reading {size} bytes from memory address 0x{address:08X} to file '{filename}'"
            )

            # Read memory using the core memory interface
            data = self.icc_mhu.memory_interface.dump(address, size)

            # Write data to file
            with open(filename, "wb") as f:
                f.write(data)

            logger.info(f"Successfully wrote {len(data)} bytes to '{filename}'")

        except Exception as e:
            if isinstance(e, (ValueError, IOError, OSError)):
                # Re-raise known exceptions as-is
                raise
            else:
                # Wrap other exceptions in RuntimeError
                raise RuntimeError(f"Failed to read memory to file: {e}") from e
