# Copyright (c) Microsoft Corporation. All rights reserved.

import os
import sys
from enum import IntEnum
import ctypes
from ctypes import Structure, c_uint16

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend(
    [
        pylibs_dir,
        current_dir,
    ]
)



class trp_node_t(ctypes.LittleEndianStructure):
    _pack_ = 1  # Force no padding
    _fields_ = [("die_id", ctypes.c_ubyte, 4), ("core_id", ctypes.c_ubyte, 4)]


class transfer_relay_protocol:

    class cpu_type(IntEnum):
        CPU_AP = 0x1
        CPU_SCP = 0x2
        CPU_MCP = 0x3
        CPU_HSP = 0x4
        CPU_TBP = 0x5
        CPU_SDM = 0x6
        CPU_CDED_SDM = 0x7
        CPU_CDED_KMP = 0x8

    class trp_status_t(IntEnum):
        TRP_STATUS_RD_DATA_VALID_MORE = 3
        TRP_STATUS_RD_DATA_VALID_LAST = 2
        TRP_STATUS_RD_DATA_NONE = 1
        TRP_STATUS_SUCCESS = 0
        TRP_STATUS_E_SIZE = -1
        TRP_STATUS_E_PARAM = -2
        TRP_STATUS_E_UNK_MSG = -3
        TRP_STATUS_E_UNK_CLIENT = -4
        TRP_STATUS_E_INCOMPLETE_HANDLER = -5
        TRP_STATUS_E_DCP_ERROR = -6

    class trp_msg_id_t(IntEnum):
        TRP_MSG_ID_DCP_FORWARD = 0x0
        TRP_MSG_ID_PACKAGE_NOTIFICATION = 0x1
        TRP_MSG_ID_READ_PACKAGE = 0x2  # command only
        TRP_MSG_ID_READ_PACKAGE_RESPONSE = 0x3
        TRP_MSG_ID_READ_PACKAGE_COMPLETE = 0x4
        TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION = 0x5
        TRP_MSG_ID_READ_INTERCORE_BLOCK = 0x6
        TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE = 0x7
        TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE = 0x8

    class trp_msg_hdr_t(ctypes.LittleEndianStructure):
        _pack_ = 1  # Force no padding
        _fields_ = [
            ("src_node", trp_node_t),
            ("dest_node", trp_node_t),
            ("trp_msg_status", ctypes.c_int8),
            ("broadcast_type", ctypes.c_uint8),
            ("mts_client_id", ctypes.c_uint16),
            ("trp_msg_id", ctypes.c_uint16),
            ("incoming_endpt", ctypes.c_uint32),  # pointer to trp_endpoint
            ("source_seq_num", ctypes.c_uint16),
            ("payload_size", ctypes.c_uint16),
        ]

        def __init__(self):
            self.src_node.die_id = 0
            self.src_node.core_id = 0
            self.dest_node.die_id = 0
            self.dest_node.core_id = 0
            self.trp_msg_status = 0
            self.broadcast_type = 0
            self.mts_client_id = 0
            self.trp_msg_id = 0
            self.incoming_endpt = 0
            self.source_seq_num = 0
            self.payload_size = 0

    class trp_block_read_req_t(Structure):
        _pack_ = 1
        _fields_ = [
            ("block_id", c_uint16),  # DCP client-specific
        ]

    class trp_msg_read_block_rsp_t(ctypes.LittleEndianStructure):
        _pack_ = 1
        _fields_ = [
            ("block_id", ctypes.c_uint16),
            ("block_version", ctypes.c_uint16),
            ("source_die_id", ctypes.c_uint8),
            ("source_core_id", ctypes.c_uint8),  # mts_platform_core_id_t
            ("reserved", ctypes.c_uint16),
            (
                "addr_offset",
                ctypes.c_uint32,
            ),  # Offset from the beginning of the client-mapped block region
            ("block_size", ctypes.c_uint32),
            ("crc", ctypes.c_uint32),  # fpfw_crc32
        ]
