# Copyright (c) Microsoft Corporation. All rights reserved.

from dataclasses import dataclass
from typing import List, Optional, Tuple
from enum import IntEnum
import logging
import ctypes


import sys, os
from pathlib import Path
from trp_lib import trp_endpoint

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend([
    pylibs_dir,
    current_dir,
])

from dcp_protocol import data_collection_protocol
from trp_protocol import transfer_relay_protocol

logger = logging.getLogger(__name__)


class dcp_msg_hdr_t(ctypes.LittleEndianStructure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("client_id", ctypes.c_uint16),
        ("msg_id", ctypes.c_uint16),
        ("seq_num", ctypes.c_uint16),
        ("msg_status", ctypes.c_int16),
        ("payload_size", ctypes.c_uint16)
    ]
    def __init__(self):
        self.client_id = 0
        self.msg_id = 0
        self.seq_num = 0
        self.msg_status = 0
        self.payload_size = 0


class dcp_commands:
    _sequence_number = 0   # Being defined here to use it for auto increment.

    @classmethod
    def _get_next_sequence_number(cls) -> int:
        """Get next sequence number and increment counter"""
        current = cls._sequence_number
        cls._sequence_number = (cls._sequence_number + 1) & 0xFFFF
        return [current & 0xFF, (current >> 8) & 0xFF]

    @staticmethod
    def create_header(msg_id: int, payload_size: int, client_id: data_collection_protocol.mts_client_id_t) -> List[int]:
        """Create DCP message header

        Args:
            msg_id: Message identifier
            payload_size: Size of payload in bytes
            client_id: Client identifier (default: 0)
        """

        header = []
        header.extend([client_id.value & 0xFF, (client_id.value >> 8) & 0xFF])  # client_id in little-endian
        header.extend([msg_id & 0xFF, (msg_id >> 8) & 0xFF])  # msg_id
        # Increment sequence number
        header.extend(dcp_commands._get_next_sequence_number()) #seq number with increment
        header.extend([0x00, 0x00])  # msg_status
        header.extend([payload_size & 0xFF, (payload_size >> 8) & 0xFF])  # payload_size
        return header

    # Using * in the method definition to enforce named parameters
    @staticmethod
    def client_enable_disable_events(*,src_endpoint: trp_endpoint, dest_die: int, dest_cpu: transfer_relay_protocol.cpu_type, client_id: data_collection_protocol.mts_client_id_t, events: list[tuple[int, int, int]]) -> None:
        """Create client enable disable events command
        NOTE: The number of events that can be sent will be limited by the CPU CLI command line max size
        Args:
            src_endpoint: endpoint that message is sent from
            dest_die: Destination die
            dest_cpu: Destination CPU
            client_id: Client identifier
            events: List of events to enable/disable
        Returns:
            None
        """
        number_of_events = len([item for item in events if isinstance(item, tuple)])


        # Create payload based on state
        dcp_msg_events = data_collection_protocol.client_events_enable_disable_msg.dcp_msg_events_enable_disable_t(number_of_events=number_of_events, events=events)

        # Convert the structure to bytes
        payload = bytearray(dcp_msg_events)

        payload_used_size = ctypes.sizeof(ctypes.c_uint16) + (number_of_events * ctypes.sizeof(data_collection_protocol.client_events_enable_disable_msg.event))

        # Create complete message with client_id
        dcp_msg_bytes = dcp_commands.create_header(
            data_collection_protocol.dcp_msg_id_t.DCP_MSG_ID_EVENTS_ENABLE_DISABLE,
            payload_used_size,
            client_id
        )
        dcp_msg_bytes.extend(payload[:payload_used_size])

        # Log raw message for debugging
        logger.debug(f"Sending client_enable_disable_events message")

        byte_response = src_endpoint.send_dcp_message(dest_die=dest_die, dest_cpu=dest_cpu, client_id=client_id, dcp_msg=dcp_msg_bytes)
        response_dcp_msg_hdr = dcp_msg_hdr_t.from_buffer_copy(byte_response)
        msg_status = data_collection_protocol.dcp_status_t(response_dcp_msg_hdr.msg_status)

        if not dcp_commands.validate_response(msg_status, logger):
            raise ValueError(f"Message Error: {msg_status.name} for command {data_collection_protocol.dcp_msg_id_t(response_dcp_msg_hdr.msg_id).name} ")

    # Using * in the method definition to enforce named parameters
    @staticmethod
    def client_start_stop(*,src_endpoint: trp_endpoint, dest_die: int, dest_cpu: transfer_relay_protocol.cpu_type, client_id: data_collection_protocol.mts_client_id_t, state: data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t) -> None:
        """Create client start/stop command
        Args:
            src_endpoint: endpoint that message is sent from
            dest_die: Destination die
            dest_cpu: Destination CPU
            client_id: Client identifier
            state: Start/Stop state to set
        Returns:
            None
        """
        # Create payload based on state
        dcp_msg_start_stop = data_collection_protocol.client_start_stop_msg.dcp_msg_start_stop_t(state=state.value)

        # Convert the structure to bytes
        payload = bytearray(dcp_msg_start_stop)

        # Create complete header with client_id
        dcp_msg_bytes = dcp_commands.create_header(
            data_collection_protocol.dcp_msg_id_t.DCP_MSG_ID_START_STOP,
            ctypes.sizeof(data_collection_protocol.client_start_stop_msg.dcp_msg_start_stop_t),
            client_id
        )
        dcp_msg_bytes.extend(payload)

        # Log raw message for debugging
        logger.debug(f"sending client_start_stop message {state.name}")

        byte_response = src_endpoint.send_dcp_message(dest_die=dest_die, dest_cpu=dest_cpu, client_id=client_id, dcp_msg=dcp_msg_bytes)
        response_dcp_msg_hdr = dcp_msg_hdr_t.from_buffer_copy(byte_response)
        msg_status = data_collection_protocol.dcp_status_t(response_dcp_msg_hdr.msg_status)

        if not dcp_commands.validate_response(msg_status, logger):
            raise ValueError(f"Message Error: {msg_status.name} for command {data_collection_protocol.dcp_msg_id_t(response_dcp_msg_hdr.msg_id).name} ")

    @staticmethod
    def client_get_state(*,src_endpoint: trp_endpoint, dest_die: int, dest_cpu: transfer_relay_protocol.cpu_type, client_id: data_collection_protocol.mts_client_id_t) -> str:
        """Get running state of client
        Args:
            src_endpoint: endpoint that message is sent from
            dest_die: Destination die
            dest_cpu: Destination CPU
            client_id: Client identifier
        Returns:
            dcp_client_state_t: state
        """
        # Create complete message with client_id
        dcp_msg_bytes = dcp_commands.create_header(
            data_collection_protocol.dcp_msg_id_t.DCP_MSG_ID_GET_STATE,
            0,
            client_id
        )

        # Log raw message for debugging
        logger.debug(f"Sending client_get_state message")

        byte_response = src_endpoint.send_dcp_message(dest_die=dest_die, dest_cpu=dest_cpu, client_id=client_id, dcp_msg=dcp_msg_bytes)
        response_dcp_msg_hdr = dcp_msg_hdr_t.from_buffer_copy(byte_response)
        msg_status = data_collection_protocol.dcp_status_t(response_dcp_msg_hdr.msg_status)

        if not dcp_commands.validate_response(msg_status, logger):
            raise ValueError(f"Message Error: {msg_status.name} for command {data_collection_protocol.dcp_msg_id_t(response_dcp_msg_hdr.msg_id).name} ")

        client_payload = data_collection_protocol.client_get_state_msg.dcp_msg_get_client_state_t.from_buffer_copy(byte_response[ctypes.sizeof(dcp_msg_hdr_t):])

        state = data_collection_protocol.client_get_state_msg.dcp_client_state_t(client_payload.state)
        logger.debug(f"Client state = : {state.name}")
        return state
    
    @staticmethod
    def client_get_manifest(*,src_endpoint: trp_endpoint, dest_die: int, dest_cpu: transfer_relay_protocol.cpu_type, client_id: data_collection_protocol.mts_client_id_t) -> str:
        """Manifest data from client
        Args:
            src_endpoint: endpoint that message is sent from
            dest_die: Destination die
            dest_cpu: Destination CPU
            client_id: Client identifier
        Returns:
            msg_status
            manifest_rsp
        """
        # Create complete message with client_id
        dcp_msg_bytes = dcp_commands.create_header(
            data_collection_protocol.dcp_msg_id_t.DCP_MSG_ID_GET_MANIFEST,
            0,
            client_id
        )

        # Log raw message for debugging
        logger.debug("Sending client_get_manifest message")

        byte_response = src_endpoint.send_dcp_message(dest_die=dest_die, dest_cpu=dest_cpu, client_id=client_id, dcp_msg=dcp_msg_bytes)
        response_dcp_msg_hdr = dcp_msg_hdr_t.from_buffer_copy(byte_response)

        msg_status = data_collection_protocol.dcp_status_t(response_dcp_msg_hdr.msg_status)

        logger.debug("Message Status for client_get_manifest : {msg_status}")

        if not dcp_commands.validate_response(msg_status, logger):
            raise ValueError(f"Message Error: {msg_status.name} for command {data_collection_protocol.dcp_msg_id_t(response_dcp_msg_hdr.msg_id).name} ")
        
        manifest_rsp = None
        manifest_rsp = data_collection_protocol.client_get_manifest_msg.dcp_msg_get_manifest_t.from_buffer_copy(byte_response[ctypes.sizeof(dcp_msg_hdr_t):])
        logger.debug(f"Response for client_get_manifest : {manifest_rsp}")

        return msg_status, manifest_rsp


    @staticmethod
    def client_read_data(*,src_endpoint: trp_endpoint, dest_die: int, dest_cpu: transfer_relay_protocol.cpu_type, client_id: data_collection_protocol.mts_client_id_t) -> str:
        """Read data from client
        Args:
            src_endpoint: endpoint that message is sent from
            dest_die: Destination die
            dest_cpu: Destination CPU
            client_id: Client identifier
        Returns:
            dcp_client_state_t: state
        """
        # Create complete message with client_id
        dcp_msg_bytes = dcp_commands.create_header(
            data_collection_protocol.dcp_msg_id_t.DCP_MSG_ID_READ_DATA,
            0,
            client_id
        )

        # Log raw message for debugging
        logger.debug(f"Sending client_read_data message")

        byte_response = src_endpoint.send_dcp_message(dest_die=dest_die, dest_cpu=dest_cpu, client_id=client_id, dcp_msg=dcp_msg_bytes)
        response_dcp_msg_hdr = dcp_msg_hdr_t.from_buffer_copy(byte_response)

        msg_status = data_collection_protocol.dcp_status_t(response_dcp_msg_hdr.msg_status)

        if not dcp_commands.validate_response(msg_status, logger):
            raise ValueError(f"Message Error: {data_collection_protocol.dcp_status_t(response_dcp_msg_hdr.msg_status).name} for command {data_collection_protocol.dcp_msg_id_t(response_dcp_msg_hdr.msg_id).name} ")

        read_data_rsp = None
        if(msg_status == data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_MORE or msg_status == data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_LAST):
            read_data_rsp = data_collection_protocol.client_read_data_msg.dcp_msg_read_data_t.from_buffer_copy(byte_response[ctypes.sizeof(dcp_msg_hdr_t):])

        return msg_status, read_data_rsp

    @staticmethod
    def client_send_read_data_complete(*,src_endpoint: trp_endpoint, dest_die: int, dest_cpu: transfer_relay_protocol.cpu_type, client_id: data_collection_protocol.mts_client_id_t, rd_data_addr_offset: ctypes.c_uint64 , rd_data_size: ctypes.c_uint64) -> str:
        """Read data from client
        Args:
            src_endpoint: endpoint that message is sent from
            dest_die: Destination die
            dest_cpu: Destination CPU
            client_id: Client identifier
        Returns:
            msg_status
        """
         # Create payload based on state
        dcp_msg_read_complete = data_collection_protocol.client_read_data_complete_msg.dcp_msg_read_data_complete_t(rd_data_addr_offset = rd_data_addr_offset, rd_data_size = rd_data_size)

        # Convert the structure to bytes
        payload = bytearray(dcp_msg_read_complete)

        # Create complete message with client_id
        dcp_msg_bytes = dcp_commands.create_header(
            data_collection_protocol.dcp_msg_id_t.DCP_MSG_ID_READ_DATA_COMPLETE,
            len(payload),
            client_id
        )

        dcp_msg_bytes.extend(payload)

        # Log raw message for debugging
        logger.debug(f"Sending client_send_read_data_complete message")

        byte_response = src_endpoint.send_dcp_message(dest_die=dest_die, dest_cpu=dest_cpu, client_id=client_id, dcp_msg=dcp_msg_bytes)
        response_dcp_msg_hdr = dcp_msg_hdr_t.from_buffer_copy(byte_response)

        msg_status = data_collection_protocol.dcp_status_t(response_dcp_msg_hdr.msg_status)

        if not dcp_commands.validate_response(msg_status, logger):
            raise ValueError(f"Message Error: {msg_status.name} for command {data_collection_protocol.dcp_msg_id_t(response_dcp_msg_hdr.msg_id).name} ")

        return msg_status

    @staticmethod
    def client_reset(*,src_endpoint: trp_endpoint, dest_die: int, dest_cpu: transfer_relay_protocol.cpu_type, client_id: data_collection_protocol.mts_client_id_t) -> str:
        """Read data from client
        Args:
            src_endpoint: endpoint that message is sent from
            dest_die: Destination die
            dest_cpu: Destination CPU
            client_id: Client identifier
        Returns:
            dcp_client_state_t: state
        """
        # Create complete message with client_id
        dcp_msg_bytes = dcp_commands.create_header(
            data_collection_protocol.dcp_msg_id_t.DCP_MSG_ID_RESET,
            0,
            client_id
        )

        # Log raw message for debugging
        logger.debug(f"Sending client_reset message")

        byte_response = src_endpoint.send_dcp_message(dest_die=dest_die, dest_cpu=dest_cpu, client_id=client_id, dcp_msg=dcp_msg_bytes, timeout_sec=30)
        response_dcp_msg_hdr = dcp_msg_hdr_t.from_buffer_copy(byte_response)

        msg_status = data_collection_protocol.dcp_status_t(response_dcp_msg_hdr.msg_status)
        logger.info(f"Msg header for reset {response_dcp_msg_hdr} and status {msg_status}")

        if not dcp_commands.validate_response(msg_status, logger):
            raise ValueError(f"Message Error: {msg_status.name} for command {data_collection_protocol.dcp_msg_id_t(response_dcp_msg_hdr.msg_id).name} ")

        return msg_status

    @staticmethod
    def validate_response(response: data_collection_protocol.dcp_status_t, logger: Optional[logging.Logger] = None) -> bool:
        """Validate DCP response
        Args:
            response: status to validate
            logger: Optional logger for error messages
        Returns:
            bool: True if response indicates success, False otherwise
        """
        if response < 0:
            if logger:
                logger.error(f"Command returned error status: {response.name} ({response})")
            return False

        return True