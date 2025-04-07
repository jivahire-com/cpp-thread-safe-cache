
# Copyright (c) Microsoft Corporation. All rights reserved.

# to  run from repo environment
# & (Join-Path ([System.Environment]::GetEnvironmentVariable("REPO_APP_PATH_python.win64", "Process")) "/tools/python.exe") tests/functional/pythia/pylibs/mts_tests/trp_lib.py
import os
import sys
import time
import ctypes
import logging


logger = logging.getLogger(__name__)

from fpfw_automation_primitives.serial.telnet import (
    Telnet_,
)
# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend([
    pylibs_dir,
    current_dir,
])

from dcp_protocol import data_collection_protocol, struct_to_hex_string, byte_list_to_hex_string
from trp_protocol import transfer_relay_protocol

from abc import ABC, abstractmethod
from enum import Enum


# Abstract Base Class (ABC)
class trp_endpoint(ABC):

    def __init__(self, source_die: ctypes.c_uint8, source_cpu: transfer_relay_protocol.cpu_type):
        self.source_die = source_die  # Store common attribute
        self.source_cpu = source_cpu
        self.sequence_number = 0
        self.default_timeout_sec = 10

    @abstractmethod
    def send_dcp_message(self, dest_die: ctypes.c_uint8, dest_cpu: transfer_relay_protocol.cpu_type, client_id: data_collection_protocol.mts_client_id_t, dcp_msg: list[int], timeout_sec: int = None) -> str:
        pass

    @abstractmethod
    def send_trp_message(self) -> str:
        pass

    def get_next_sequence_number(self) -> int:
        """Get next sequence number and increment counter"""
        self.sequence_number = (self.sequence_number + 1) & 0x1FF
        return self.sequence_number

class mts_cli_trp_endpoint(trp_endpoint):
    def __init__(self, source_comm_channel, source_die: ctypes.c_uint8, source_cpu: transfer_relay_protocol.cpu_type):
        super().__init__(source_die, source_cpu )
        self.source_comm_channel = source_comm_channel

    def send_trp_message(self) -> str:
        self.source_comm_channel.write(write_string="?\n")
        response = self.source_comm_channel.read_until(key="hm", timeout_seconds=2)
        return response

    def send_dcp_message(self, dest_die: ctypes.c_uint8, dest_cpu: transfer_relay_protocol.cpu_type, client_id: data_collection_protocol.mts_client_id_t, dcp_msg: list[int], timeout_sec: int = None) -> str:

        trp_msg_hdr = transfer_relay_protocol.trp_msg_hdr_t()
        trp_msg_hdr.src_node.die_id = self.source_die
        trp_msg_hdr.src_node.core_id = self.source_cpu.value
        trp_msg_hdr.dest_node.die_id = dest_die
        trp_msg_hdr.dest_node.core_id = dest_cpu.value
        trp_msg_hdr.mts_client_id = client_id.value
        trp_msg_hdr.trp_msg_id = transfer_relay_protocol.trp_msg_id_t.TRP_MSG_ID_DCP_FORWARD.value
        trp_msg_hdr.source_seq_num = self.get_next_sequence_number()
        trp_msg_hdr.payload_size = len(dcp_msg)


        send_str = f"mts trp_send " + struct_to_hex_string(trp_msg_hdr) + byte_list_to_hex_string(dcp_msg)

        logger.debug(f"\nSending DCP in TRP message: {send_str}\n")
        self.source_comm_channel.write_line(write_string=send_str)
        try:
            cmd_timeout_sec = timeout_sec if timeout_sec is not None else self.default_timeout_sec
            response = self.source_comm_channel.read_until(key="TrpRx", timeout_seconds=cmd_timeout_sec)
        except Exception as e:
            logger.error(f"Error while reading response: {e}")
            response = ""  # or handle as appropriate

        rsp_index = response.find("Rsp: ")
        if (rsp_index == -1):
            raise ValueError("TRP Response not found in {response}")

        response_str = response[rsp_index + 4:].strip()
        response_str = response_str.split('\n')[0]  # Take only the part before the newline

        # Split the string into individual components and convert to list of integers
        response_list = [int(x) for x in response_str.split()]

         # Convert response_list to byte array
        response_bytes = bytearray(response_list)
        logger.debug(f"\nDCP in TRP response: {' '.join(f'{b:02X}' for b in response_bytes)}\n")

        # Log the TRP header fields
        logger.debug(f"Response TRP Header")
        logger.debug(f"TRP Header - Source Die ID: {trp_msg_hdr.src_node.die_id}")
        logger.debug(f"TRP Header - Source Core ID: {trp_msg_hdr.src_node.core_id}")
        logger.debug(f"TRP Header - Destination Die ID: {trp_msg_hdr.dest_node.die_id}")
        logger.debug(f"TRP Header - Destination Core ID: {trp_msg_hdr.dest_node.core_id}")
        logger.debug(f"TRP Header - Message status: {trp_msg_hdr.trp_msg_status:x}")
        logger.debug(f"TRP Header - DCP Client ID: {trp_msg_hdr.mts_client_id}")
        logger.debug(f"TRP Header - TRP Message ID: {trp_msg_hdr.trp_msg_id:x}")
        logger.debug(f"TRP Header - Source Sequence Number: {trp_msg_hdr.source_seq_num:x}")
        logger.debug(f"TRP Header - Payload Size: {trp_msg_hdr.payload_size:x}")

        # log the DCP header fields
        dcp_msg = response_bytes[ctypes.sizeof(transfer_relay_protocol.trp_msg_hdr_t):]
        dcp_msg_hdr = data_collection_protocol.dcp_msg_hdr_t.from_buffer_copy(dcp_msg)
        logger.debug(f"Response DCP Header")
        logger.debug(f"DCP Header - Client ID: {dcp_msg_hdr.client_id}")
        logger.debug(f"DCP Header - Message ID: {dcp_msg_hdr.msg_id:x}")
        logger.debug(f"DCP Header - Sequence Number: {dcp_msg_hdr.seq_num:x}")
        logger.debug(f"DCP Header - Message Status: {dcp_msg_hdr.msg_status:x}")
        logger.debug(f"DCP Header - Payload Size: {dcp_msg_hdr.payload_size:x}")


        if(len(response_bytes) < (ctypes.sizeof(transfer_relay_protocol.trp_msg_hdr_t) + ctypes.sizeof(data_collection_protocol.dcp_msg_hdr_t))):
            raise ValueError("Response is too short to contain trp_msg_hdr_t and dcp_msg_hdr_t, {response_bytes}")


        return dcp_msg
