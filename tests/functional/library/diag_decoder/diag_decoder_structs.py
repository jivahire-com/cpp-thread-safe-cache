# Copyright (c) Microsoft Corporation. All rights reserved.

import ctypes
from enum import IntEnum, auto


# Enums
class diag_manifest_parser_type(IntEnum):
    diag_manifest_parser_packed = 1
    diag_manifest_parser_health_collector = 2


class diag_manifest_set_header_version(IntEnum):
    diag_manifest_set_header_version_v1 = 1
    diag_manifest_set_header_version_v2 = 2


class diag_payload_parser_type(IntEnum):
    diag_payload_parser_telemetry = 1
    diag_payload_parser_trace_device = 2
    diag_payload_parser_trace_core = 3
    diag_payload_parser_hsp_trace = 4
    diag_payload_parser_health_collector = 5
    diag_payload_parser_invalid = 6


class diag_pwr_telemetry_payload_parser_version(IntEnum):
    diag_pwr_telemetry_payload_parser_v1 = 1
    diag_pwr_telemetry_payload_parser_v2 = 2


class diag_trace_payload_parser_version(IntEnum):
    diag_trace_payload_parser_version_core_payload = 1
    diag_trace_payload_parser_version_device_payload = 2


class diag_hsp_payload_parser_version(IntEnum):
    diag_hsp_payload_parser_version = 1


class diag_manifest_set_v1_header(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("manifest_set_header_version", ctypes.c_uint32),  # diag_manifest_set_header_version_t
        ("manifest_count", ctypes.c_uint32),
        ("payload", ctypes.c_uint8 * 0),  # Flexible array member
    ]

class diag_manifest_set_v2_header(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("manifest_set_header_version", ctypes.c_uint32),  # diag_manifest_set_header_version_t
        ("manifest_count", ctypes.c_uint32),
        ("sentinel", ctypes.c_uint32),  # Provides quick check to see if header looks valid
        ("manifest_set_size", ctypes.c_uint32),  # Everything after this header
        ("crc32", ctypes.c_uint32),
        ("payload", ctypes.c_uint8 * 0),  # Flexible array member
    ]

class diag_manifest_header(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("manifest_parser_type", ctypes.c_uint32),  # diag_manifest_parser_type_t
        ("manifest_size", ctypes.c_uint32),
        ("payload", ctypes.c_uint8 * 0),  # Flexible array member
    ]

class diag_packed_manifest_header(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("manifest_id", ctypes.c_uint32),  # Placeholder for FPFW_ET_MANIFEST_ID
        ("provider_metadata_size", ctypes.c_uint32),
        ("event_metadata_size", ctypes.c_uint32),
        ("payload", ctypes.c_uint8 * 0),  # Flexible array member
    ]

class diag_decoder_payload_header(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("payload_parser_version", ctypes.c_uint32),  # Specific to diag_payload_parser_type_t
        ("payload_parser_type", ctypes.c_uint32),  # diag_payload_parser_type_t
        ("payload", ctypes.c_uint8 * 0),  # Flexible array member
    ]

class fp_fw_et_manifest_id(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("data1", ctypes.c_uint32),  # 32-bit unsigned integer
        ("data2", ctypes.c_uint16),  # 16-bit unsigned integer
        ("data3", ctypes.c_uint16),  # 16-bit unsigned integer
        ("data4", ctypes.c_uint8 * 8),  # Array of 8 unsigned 8-bit integers
    ]
