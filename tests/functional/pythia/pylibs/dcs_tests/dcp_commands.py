# Copyright (c) Microsoft Corporation. All rights reserved.

from dataclasses import dataclass
from typing import List, Optional, Tuple
from enum import IntEnum
import logging

import sys, os
from pathlib import Path

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend([
    pylibs_dir,
    current_dir,
])

logger = logging.getLogger(__name__)

@dataclass
class DcpResponse:
    """Response from a DCP message"""
    status: int
    payload: Optional[List[int]] = None
    error: Optional[str] = None

class DataCollectionProtocol:
    """Constants defining the Data Collection Protocol Specification"""
    _sequence_number = 0   # Being defined here to use it for auto increment.

    class MessageId(IntEnum):
        """Message IDs as defined in protocol spec"""
        CLIENT_GET_CAPABILITIES = 0x0000
        CLIENT_GET_STATE = 0x0001
        CLIENT_GET_SCHEMA = 0x0002
        CLIENT_EVENTS_ENABLE_DISABLE = 0x0003
        CLIENT_START_STOP = 0x0005
        CLIENT_READ_DATA = 0x0007
        CLIENT_READ_DATA_COMPLETE = 0x0008
        CLIENT_RESET = 0x000A
        CLIENT_NOTIFICATION = 0x000B
        CLIENT_GET_PLATFORM_INFORMATION = 0x000C

    class Status(IntEnum):
        """Status codes as defined in protocol spec"""
        DATA_COLLECTION_SUCCESS = 0x0000
        DATA_COLLECTION_RD_DATA_NONE = 0x0001
        DATA_COLLECTION_RD_DATA_VALID_LAST = 0x0002
        DATA_COLLECTION_RD_DATA_VALID_MORE = 0x0003
        DATA_COLLECTION_E_SIZE = 0xFFFF
        DATA_COLLECTION_E_PARAM = 0xFFFE
        DATA_COLLECTION_E_BUSY = 0xFFFD
        DATA_COLLECTION_E_UNK_MSG = 0xFFFC
        DATA_COLLECTION_E_UNK_CLIENT = 0xFFFB
        DATA_COLLECTION_E_INCOMPLETE_HANDLER = 0xFFFA
        DATA_COLLECTION_E_DEPRECATED_MSG = 0xFFF9
        DATA_COLLECTION_E_BUFFER_DISCARDED = 0xFFF8
        DATA_COLLECTION_E_UNSUPPORTED_MSG = 0xFFF7

    class ClientEventsEnableDisable:
        """CLIENT_EVENTS_ENABLE_DISABLE protocol constants"""
        MAX_EVENTS = 64
        
        class PayloadFormat:
            """Payload format as per protocol spec"""
            NUM_EVENTS_SIZE = 2
            PROVIDER_ID_SIZE = 2
            EVENT_ID_SIZE = 2
            EVENT_STATE_SIZE = 2
            EVENT_ENTRY_SIZE = PROVIDER_ID_SIZE + EVENT_ID_SIZE + EVENT_STATE_SIZE
        
        class EventState(IntEnum):
            """Event states as defined in protocol spec"""
            EVENT_DISABLED = 0x0000
            EVENT_ENABLED = 0x0001

    class ClientStartStop:
        """CLIENT_START_STOP protocol constants"""
        class State(IntEnum):
            """States as defined in protocol spec"""
            STOP = 0x0000_0000
            START = 0x0000_0001

    @staticmethod
    def to_hex_string(msg: List[int]) -> str:
        """Convert message bytes to hex string format
        
        Args:
            msg: List of bytes to convert
            
        Returns:
            str: Space-separated hex string (e.g., "0x00 0x01 0x02")
        """
        return " ".join(f"0x{b:02x}" for b in msg)
    
    @classmethod
    def _get_next_sequence_number(cls) -> int:
        """Get next sequence number and increment counter"""
        current = cls._sequence_number
        cls._sequence_number = (cls._sequence_number + 1) & 0xFFFF
        return [current & 0xFF, (current >> 8) & 0xFF]

    @staticmethod
    def create_header(msg_id: int, payload_size: int, client_id: int = 0) -> List[int]:
        """Create DCP message header
        
        Args:
            msg_id: Message identifier
            payload_size: Size of payload in bytes
            client_id: Client identifier (default: 0)
        """
        header = []
        header.extend([client_id & 0xFF, (client_id >> 8) & 0xFF])  # client_id in little-endian
        header.extend([msg_id & 0xFF, (msg_id >> 8) & 0xFF])  # msg_id
        # Increment sequence number
        header.extend(DataCollectionProtocol._get_next_sequence_number()) #seq number with increment 
        header.extend([0x00, 0x00])  # msg_status
        header.extend([payload_size & 0xFF, (payload_size >> 8) & 0xFF])  # payload_size
        return header

    # Using * in the method definition to enforce named parameters
    @staticmethod
    def client_start_stop(*, client_id: int, state: ClientStartStop.State) -> str:
        """Create client start/stop command
        
        Args:
            client_id: Client identifier
            state: Start/Stop state to set
            
        Returns:
            str: Hex formatted command string
            
        Examples:
            For START command (0x00000001):
            msg = [
                # Header (10 bytes):
                0x00, 0x00,             # client_id (2 bytes)
                0x05, 0x00,             # msg_id (2 bytes, 0x0005 in little-endian)
                0x00, 0x00,             # seq_num (2 bytes)
                0x00, 0x00,             # msg_status (2 bytes)
                0x04, 0x00,             # payload_size (2 bytes, 4 in little-endian)
                
                # Payload (4 bytes):
                0x01, 0x00, 0x00, 0x00  # START state (0x00000001 in little-endian)
            ]
        """
        # Create payload based on state
        if state == DataCollectionProtocol.ClientStartStop.State.START:
            payload = [0x01, 0x00, 0x00, 0x00]  # START = 0x00000001 in little-endian
        else:
            payload = [0x00, 0x00, 0x00, 0x00]  # STOP = 0x00000000 in little-endian
        
        # Create complete message with client_id
        msg = DataCollectionProtocol.create_header(
            DataCollectionProtocol.MessageId.CLIENT_START_STOP,
            len(payload),
            client_id
        )
        msg.extend(payload)
        
        # Log raw message for debugging
        logger.debug(f"Raw message bytes: {msg}")
        
        return DataCollectionProtocol.to_hex_string(msg)

    # TODO: Add more protocols and uncomment once start stop is reviewed.
    # @staticmethod
    # def events_enable_disable(events: List[dict]) -> Tuple[int, List[int], str]:
    

    @staticmethod
    def validate_response(response: DcpResponse, logger: Optional[logging.Logger] = None) -> bool:
        """Validate DCP response
        
        Args:
            response: DcpResponse to validate
            logger: Optional logger for error messages
            
        Returns:
            bool: True if response indicates success, False otherwise
        """
        if response.error:
            if logger:
                logger.error(f"Command failed: {response.error}")
            return False
        
        if response.status != DataCollectionProtocol.Status.DATA_COLLECTION_SUCCESS:
            if logger:
                logger.error(f"Command returned error status: 0x{response.status:x}")
            return False
            
        return True