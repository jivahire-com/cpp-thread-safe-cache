# Copyright (c) Microsoft Corporation. All rights reserved.

import os
import sys
from enum import IntEnum
import ctypes

# Add paths for both package and direct imports
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
sys.path.extend([
    pylibs_dir,
    current_dir,
])

from dcp_protocol import data_collection_protocol, struct_to_hex_string, byte_list_to_hex_string

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
        TRP_STATUS_RD_DATA_VALID_MORE   = 3
        TRP_STATUS_RD_DATA_VALID_LAST   = 2
        TRP_STATUS_RD_DATA_NONE         = 1
        TRP_STATUS_SUCCESS              = 0
        TRP_STATUS_E_SIZE               = -1
        TRP_STATUS_E_PARAM              = -2
        TRP_STATUS_E_UNK_MSG            = -3
        TRP_STATUS_E_UNK_CLIENT         = -4
        TRP_STATUS_E_INCOMPLETE_HANDLER = -5
        TRP_STATUS_E_DCP_ERROR          = -6

    class trp_msg_id_t(IntEnum):
        TRP_MSG_ID_DCP_FORWARD                      = 0xD000
        TRP_MSG_ID_PACKAGE_NOTIFICATION             = 0xD001
        TRP_MSG_ID_READ_PACKAGE                     = 0xD002 # command only
        TRP_MSG_ID_READ_PACKAGE_RESPONSE            = 0xD003
        TRP_MSG_ID_READ_PACKAGE_COMPLETE            = 0xD004
        TRP_MSG_ID_INTERCORE_BLOCK_NOTIFICATION     = 0xD005
        TRP_MSG_ID_READ_INTERCORE_BLOCK             = 0xD006
        TRP_MSG_ID_READ_INTERCORE_BLOCK_RESPONSE    = 0xD007
        TRP_MSG_ID_READ_INTERCORE_BLOCK_COMPLETE    = 0xD008


    class trp_msg_hdr_t(ctypes.LittleEndianStructure):
        _pack_ = 1  # Force no padding
        _fields_ = [
            ("source_die_id", ctypes.c_uint8),
            ("source_cpu_id", ctypes.c_uint8),
            ("dest_die_id", ctypes.c_uint8),
            ("dest_cpu_id", ctypes.c_uint8),
            ("dcp_client_id", ctypes.c_uint16),
            ("trp_msg_id", ctypes.c_uint16),
            ("trp_msg_status", ctypes.c_int16),
            ("source_seq_num", ctypes.c_uint16),
            ("incoming_endpt", ctypes.c_uint32),    # pointer to trp_endpoint
            ("broadcast_type", ctypes.c_uint16),
            ("payload_size", ctypes.c_uint16)
        ]
        def __init__(self):
            self.source_die_id = 0
            self.source_cpu_id = 0
            self.dest_die_id = 0
            self.dest_cpu_id = 0
            self.dcp_client_id = 0
            self.trp_msg_id = 0
            self.trp_msg_status = 0
            self.source_seq_num = 0
            self.incoming_endpt = 0
            self.broadcast_type = 0
            self.payload_size = 0