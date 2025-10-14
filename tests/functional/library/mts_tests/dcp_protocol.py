# Copyright (c) Microsoft Corporation. All rights reserved.

import ctypes
from enum import IntEnum


# Type aliases for better clarity
UInt16 = int  # Constrained to 0-65535 range


def struct_to_hex_string(struct_instance):
    """Convert a ctypes structure to a decimal stream."""
    byte_stream = bytes(struct_instance)  # Convert struct to bytes
    return " ".join(f"{x:d}" for x in byte_stream) + " "


def byte_list_to_hex_string(byte_list: list[int]) -> str:
    """Convert a list of bytes to a formatted hex string with 0x prefix."""
    if any(byte < 0 or byte > 255 for byte in byte_list):
        raise ValueError("All values must be in the range 0-255 (0x00-0xFF)")

    return " ".join(f"{byte:d}" for byte in byte_list) + " "


class data_collection_protocol:
    """Constants defining the Data Collection Protocol Specification"""

    class mts_client_id_t(IntEnum):
        MTS_CLIENT_ID_DCP_SVC = 0
        MTS_CLIENT_ID_PWR_INST_TELEM = 1
        MTS_CLIENT_ID_EVENT_TRACE = 2
        MTS_CLIENT_ID_MAX = 3

    class dcp_msg_id_t(IntEnum):
        DCP_MSG_ID_GET_CAPABILITIES = 0x0  # Get capabilities
        DCP_MSG_ID_GET_STATE = 0x1  # Get state of the client
        DCP_MSG_ID_GET_MANIFEST = 0x2  # Get manifest
        DCP_MSG_ID_EVENTS_ENABLE_DISABLE = 0x3  # Enable/Disable events
        DCP_MSG_ID_EVENTS_DISABLE = 0x4  # Deprecated
        DCP_MSG_ID_START_STOP = 0x5  # Start/Stop data collection
        DCP_MSG_ID_STOP = 0x6  # Deprecated - Keeping for backward compatibility
        DCP_MSG_ID_READ_DATA = 0x7  # Read data
        DCP_MSG_ID_READ_DATA_COMPLETE = 0x8  # Read data complete
        DCP_MSG_ID_UTC_SYNC = 0x9  # Deprecated - Keeping for backward compatibility
        DCP_MSG_ID_RESET = 0xA  # Reset client
        DCP_MSG_ID_NOTIFICATION = 0xB  # Notification
        DCP_MSG_ID_GET_PLAT_INFO = 0xC  # Get platform information
        DCP_MSG_ID_MAX = 0xD

    class dcp_status_t(IntEnum):
        """Status codes as defined in protocol spec"""

        DATA_COLLECTION_SUCCESS = 0x0000
        DATA_COLLECTION_RD_DATA_NONE = 0x0001
        DATA_COLLECTION_RD_DATA_VALID_LAST = 0x0002
        DATA_COLLECTION_RD_DATA_VALID_MORE = 0x0003
        DATA_COLLECTION_E_SIZE = -1
        DATA_COLLECTION_E_PARAM = -2
        DATA_COLLECTION_E_BUSY = -3
        DATA_COLLECTION_E_UNK_MSG = -4
        DATA_COLLECTION_E_UNK_CLIENT = -5
        DATA_COLLECTION_E_INCOMPLETE_HANDLER = -6
        DATA_COLLECTION_E_DEPRECATED_MSG = -7
        DATA_COLLECTION_E_BUFFER_DISCARDED = -8
        DATA_COLLECTION_E_UNSUPPORTED_MSG = -9

    class dcp_msg_hdr_t(ctypes.LittleEndianStructure):
        _pack_ = 1  # Force no padding
        _fields_ = [
            ("client_id", ctypes.c_uint16),
            ("msg_id", ctypes.c_uint16),
            ("seq_num", ctypes.c_uint16),
            ("msg_status", ctypes.c_int16),
            ("payload_size", ctypes.c_uint16),
        ]

        def __init__(self):
            self.client_id = 0
            self.msg_id = 0
            self.seq_num = 0
            self.msg_status = 0
            self.payload_size = 0

    class client_events_enable_disable_msg:
        """DCP_MSG_ID_EVENTS_ENABLE_DISABLE protocol constants"""

        MAX_EVENTS = 64

        class dcp_events_enable_state_t(IntEnum):
            """Event states as defined in protocol spec"""

            DCP_EVENTS_ENABLE_STATE_DISABLE = 0x0000
            DCP_EVENTS_ENABLE_STATE_ENABLE = 0x0001

        class event(ctypes.LittleEndianStructure):
            _pack_ = 1  # Force no padding
            _fields_ = [
                ("provider_id", ctypes.c_uint16),
                ("event_id", ctypes.c_uint16),
                ("state", ctypes.c_uint16),  # dcp_events_enable_state_t
            ]

        class dcp_msg_events_enable_disable_t(ctypes.LittleEndianStructure):
            """DCP_MSG_ID_EVENTS_ENABLE_DISABLE message format as per protocol spec"""

            _pack_ = 1  # Force no padding

    class client_start_stop_msg:

        class dcp_start_stop_state_t(IntEnum):
            """States as defined in protocol spec"""

            DCP_START_STOP_STATE_STOP = 0x0000_0000
            DCP_START_STOP_STATE_START = 0x0000_0001

        class dcp_msg_start_stop_t(ctypes.LittleEndianStructure):
            """CLIENT_START_STOP message format as per protocol spec"""

            _pack_ = 1  # Force no padding
            _fields_ = [
                ("state", ctypes.c_uint32),  # dcp_start_stop_state_t
            ]

            def __init__(
                self,
                state: "data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t",
            ):
                super().__init__()
                self.state = state

    class client_get_state_msg:

        class dcp_client_state_t(IntEnum):
            """States as defined in protocol spec"""

            DCP_CLIENT_STATE_STOPPED = 0x0000_0000
            DCP_CLIENT_STATE_RUNNING = 0x0000_0001

        class dcp_msg_get_client_state_t(ctypes.LittleEndianStructure):
            """CLIENT_START_STOP message format as per protocol spec"""

            _pack_ = 1  # Force no padding
            _fields_ = [
                ("state", ctypes.c_uint32),  # dcp_client_state_t
            ]

            def __init__(
                self,
                state: "data_collection_protocol.client_get_state_msg.dcp_msg_get_client_state_t",
            ):
                super().__init__()
                self.state = state

    class client_read_data_msg:

        class dcp_msg_read_data_t(ctypes.LittleEndianStructure):
            """CLIENT_READ_DATA message format as per protocol spec"""

            _pack_ = 1  # Force no padding
            _fields_ = [
                ("physical_start_addr", ctypes.c_uint64),
                ("physical_buffer_size", ctypes.c_uint64),
                ("rd_data_addr_offset", ctypes.c_uint64),
                ("rd_data_size", ctypes.c_uint64),
                ("crc", ctypes.c_uint32),
            ]

            def __init__(
                self,
                physical_start_addr: ctypes.c_uint64,
                physical_buffer_size: ctypes.c_uint64,
                rd_data_addr_offset: ctypes.c_uint64,
                rd_data_size: ctypes.c_uint64,
                crc: ctypes.c_uint32,
            ):
                super().__init__()
                self.physical_start_addr = physical_start_addr
                self.physical_buffer_size = physical_buffer_size
                self.rd_data_addr_offset = rd_data_addr_offset
                self.rd_data_size = rd_data_size
                self.crc = crc

    class client_get_manifest_msg:

        class dcp_msg_get_manifest_t(ctypes.LittleEndianStructure):
            """CLIENT_GET_MANIFEST message format as per protocol spec"""

            _pack_ = 1  # Force no padding
            _fields_ = [
                ("physical_start_addr", ctypes.c_uint64),
                ("start_addr_offset", ctypes.c_uint64),
                ("total_size", ctypes.c_uint64),
            ]

            def __init__(
                self,
                physical_start_addr: ctypes.c_uint64,
                start_addr_offset: ctypes.c_uint64,
                total_size: ctypes.c_uint64,
            ):
                super().__init__()
                self.physical_start_addr = physical_start_addr
                self.start_addr_offset = start_addr_offset
                self.total_size = total_size

    class client_read_data_complete_msg:

        class dcp_msg_read_data_complete_t(ctypes.LittleEndianStructure):
            """CLIENT_READ_DATA_COMPLETE message format as per protocol spec"""

            _pack_ = 1  # Force no padding
            _fields_ = [
                ("reserved1", ctypes.c_uint32),
                ("rd_data_addr_offset", ctypes.c_uint64),
                ("rd_data_size", ctypes.c_uint64),
                ("reserved2", ctypes.c_uint32),
            ]

            def __init__(
                self,
                rd_data_addr_offset: ctypes.c_uint64,
                rd_data_size: ctypes.c_uint64,
            ):
                super().__init__()
                self.rd_data_addr_offset = rd_data_addr_offset
                self.rd_data_size = rd_data_size
                self.reserved1 = 0
                self.reserved2 = 0

    class dcp_msg_get_plat_info_t(ctypes.LittleEndianStructure):
        """Platform information structure"""

        COBALT_200 = 2
        _pack_ = 1
        _fields_ = [
            ("dcp_ver_major", ctypes.c_uint32),
            ("dcp_ver_minor", ctypes.c_uint32),
            ("dcp_ver_patch", ctypes.c_uint32),
            ("ifwi_ver_major", ctypes.c_uint32),
            ("ifwi_ver_minor", ctypes.c_uint32),
            ("ifwi_ver_patch", ctypes.c_uint32),
            ("ifwi_ver_rev", ctypes.c_uint32),
            ("plat_id", ctypes.c_uint64),
        ]

        def __init__(
            self,
            dcp_ver_major,
            dcp_ver_minor,
            dcp_ver_patch,
            ifwi_ver_major,
            ifwi_ver_minor,
            ifwi_ver_patch,
            ifwi_ver_rev,
            plat_id,
        ):
            super().__init__()
            self.dcp_ver_major = dcp_ver_major
            self.dcp_ver_minor = dcp_ver_minor
            self.dcp_ver_patch = dcp_ver_patch
            self.ifwi_ver_major = ifwi_ver_major
            self.ifwi_ver_minor = ifwi_ver_minor
            self.ifwi_ver_patch = ifwi_ver_patch
            self.ifwi_ver_rev = ifwi_ver_rev
            self.plat_id = plat_id

    class dcp_msg_get_caps_t(ctypes.LittleEndianStructure):
        _pack_ = 1
        _fields_ = [
            ("DCP_MSG_ID_GET_CAPABILITIES", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_GET_STATE", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_GET_MANIFEST", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_EVENTS_ENABLE_DISABLE", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_EVENTS_DISABLE", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_START_STOP", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_STOP", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_READ_DATA", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_READ_DATA_COMPLETE", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_UTC_SYNC", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_RESET", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_NOTIFICATION", ctypes.c_uint32, 1),
            ("DCP_MSG_ID_GET_PLAT_INFO", ctypes.c_uint32, 1),
            ("reserved", ctypes.c_uint32, 19),
        ]

        def __init__(
            self,
            get_cap,
            get_state,
            get_manifest,
            events_enable_disable,
            events_disable,
            start_stop,
            stop,
            read_data,
            read_data_complete,
            utc_sync,
            reset,
            notification,
            get_plat_info,
            reserved,
        ):
            super().__init__()
            self.DCP_MSG_ID_GET_CAPABILITIES = get_cap
            self.DCP_MSG_ID_GET_STATE = get_state
            self.DCP_MSG_ID_GET_MANIFEST = get_manifest
            self.DCP_MSG_ID_EVENTS_ENABLE_DISABLE = events_enable_disable
            self.DCP_MSG_ID_EVENTS_DISABLE = events_disable
            self.DCP_MSG_ID_START_STOP = start_stop
            self.DCP_MSG_ID_STOP = stop
            self.DCP_MSG_ID_READ_DATA = read_data
            self.DCP_MSG_ID_READ_DATA_COMPLETE = read_data_complete
            self.DCP_MSG_ID_UTC_SYNC = utc_sync
            self.DCP_MSG_ID_RESET = reset
            self.DCP_MSG_ID_NOTIFICATION = notification
            self.DCP_MSG_ID_GET_PLAT_INFO = get_plat_info
            self.reserved = reserved


# these are required to be defined outside of the class definition because it references
# the class itself before it is fully constructed
data_collection_protocol.client_events_enable_disable_msg.dcp_msg_events_enable_disable_t._fields_ = [
    ("number_of_events", ctypes.c_uint16),
    (
        "events",
        data_collection_protocol.client_events_enable_disable_msg.event
        * data_collection_protocol.client_events_enable_disable_msg.MAX_EVENTS,
    ),  # array of Event structs
]


def event_list_init(self, number_of_events: int, events: list[tuple[int, int, int]]):
    if (
        number_of_events
        > data_collection_protocol.client_events_enable_disable_msg.MAX_EVENTS
    ):
        raise ValueError(
            f"number_of_events, {number_of_events}, cannot exceed "
            f"{data_collection_protocol.client_events_enable_disable_msg.MAX_EVENTS}"
        )
    self.number_of_events = number_of_events

    # Default to an empty list if no events are provided
    if events is None:
        events = []

    # Validate that all event tuple values fit in ctypes.c_uint16 range (0-65535)
    for i, event in enumerate(events):
        if len(event) != 3:
            raise ValueError(
                f"Event {i} must be a tuple of exactly 3 integers: (provider_id, event_id, state)"
            )

        provider_id, event_id, state = event
        for field_name, value in [
            ("provider_id", provider_id),
            ("event_id", event_id),
            ("state", state),
        ]:
            if not isinstance(value, int):
                raise TypeError(
                    f"Event {i} {field_name} must be an integer, got {type(value).__name__}"
                )
            if not (0 <= value <= 0xFFFF):
                raise ValueError(
                    f"Event {i} {field_name} value {value} must be in range 0-65535 "
                    f"(0x0000-0xFFFF) for ctypes.c_uint16"
                )

    # Ensure events list has exactly MAX_EVENTS entries, filling with (0, 0, 0) if needed
    events = (
        list(events)
        + [(0, 0, 0)]
        * data_collection_protocol.client_events_enable_disable_msg.MAX_EVENTS
    )  # Ensure it's a list before concatenation
    events = events[
        : data_collection_protocol.client_events_enable_disable_msg.MAX_EVENTS
    ]  # Trim to exactly MAX_EVENTS elements

    # Assign the event values to the struct array
    for i in range(
        data_collection_protocol.client_events_enable_disable_msg.MAX_EVENTS
    ):
        provider_id, event_id, state = events[i]
        self.events[i] = (
            data_collection_protocol.client_events_enable_disable_msg.event(
                provider_id, event_id, state
            )
        )


data_collection_protocol.client_events_enable_disable_msg.dcp_msg_events_enable_disable_t.__init__ = (
    event_list_init
)
