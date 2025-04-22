# Copyright (c) Microsoft Corporation. All rights reserved.

import ctypes
from enum import IntEnum, auto
import sys
from pathlib import Path


sys.path.append(str(Path(__file__).parent.parent / 'diag_decoder'))

import diag_decoder_structs as dd

class telemetry_payload_header(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("manifest_id", dd.fp_fw_et_manifest_id),  # Reference to FPFW_ET_MANIFEST_ID
        ("timestamp_us", ctypes.c_uint64),  # Timestamp when the package was created
        ("timestamp_utc", ctypes.c_uint64),
        ("source_die", ctypes.c_uint32),
        ("package_number", ctypes.c_uint32),
        ("number_of_records", ctypes.c_uint32),
        ("package_payload_size", ctypes.c_uint32),
    ]

class telemetry_package_hdr(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("decoder_header", dd.diag_decoder_payload_header),  # Reference to diag_decoder_payload_header_t
        ("payload_header", telemetry_payload_header),  # Reference to telemetry_payload_header_t
    ]

class telemetry_collection_hdr(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("provider_id", ctypes.c_uint16),
        ("element_id", ctypes.c_uint16),
        ("collection_id", ctypes.c_uint16),
        ("number_of_elements", ctypes.c_uint16),
        ("collection_payload_size", ctypes.c_uint32),  # Minus the collection header size
    ]

class telemetry_record_hdr(ctypes.Structure):
    _pack_ = 1  # Force no padding
    _fields_ = [
        ("timestamp_us", ctypes.c_uint64),  # Timestamp when the record was created
        ("record_number", ctypes.c_uint32),
        ("number_of_collections", ctypes.c_uint32),
        ("record_payload_size", ctypes.c_uint32),  # Minus the record header size
    ]