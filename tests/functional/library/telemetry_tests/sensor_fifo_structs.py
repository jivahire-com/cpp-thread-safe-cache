# Copyright (c) Microsoft Corporation. All rights reserved.

import ctypes
from enum import IntEnum, auto

# Enums
class sensor_telem_type(IntEnum):
    temperature = 0
    voltage = 1
    current = 2

class pstate_throttle_status_codes(IntEnum):
    no_throttle = 0
    current_throttle_start = 1
    temp_throttle_start = 2
    rack_throttle_start = 3
    sys_frc_pmin_throttle_start = 4
    adpt_clk_throttle_start = 5
    current_throttle_end = 6
    temp_throttle_end = 7
    rack_throttle_end = 8
    sys_frc_pmin_throttle_end = 9
    adpt_clk_throttle_end = 10
    current_throttle_overrun = 11
    adpt_clk_throttle_overrun = 12


class temp_max_data(ctypes.Structure):
    _fields_ = [
        ("temp_valid", ctypes.c_uint64, 8),
        ("max_id", ctypes.c_uint64, 8),
        ("max_temp", ctypes.c_uint64, 16),
        ("core0", ctypes.c_uint64, 16),
        ("core1", ctypes.c_uint64, 16),
    ]


class temp_full_data0(ctypes.Structure):
    _fields_ = [
        ("temp_valid", ctypes.c_uint64, 8),
        ("max_id", ctypes.c_uint64, 8),
        ("max_temp", ctypes.c_uint64, 16),
        ("core0", ctypes.c_uint64, 16),
        ("core1", ctypes.c_uint64, 16),
    ]


class temp_full_data1(ctypes.Structure):
    _fields_ = [
        ("temp0", ctypes.c_uint64, 16),
        ("temp1", ctypes.c_uint64, 16),
        ("temp2", ctypes.c_uint64, 16),
        ("temp3", ctypes.c_uint64, 16),
    ]


class temp_full_data2(ctypes.Structure):
    _fields_ = [
        ("temp4", ctypes.c_uint64, 16),
        ("temp5", ctypes.c_uint64, 16),
        ("temp6", ctypes.c_uint64, 16),
        ("temp7", ctypes.c_uint64, 16),
    ]


class volt_data(ctypes.Structure):
    _fields_ = [
        ("vcore0", ctypes.c_uint64, 16),
        ("vcore1", ctypes.c_uint64, 16),
        ("vcpu", ctypes.c_uint64, 16),
        ("vsys", ctypes.c_uint64, 16),
    ]


class current_data(ctypes.Structure):
    _fields_ = [
        ("avg", ctypes.c_uint64, 8),
        ("min", ctypes.c_uint64, 8),
        ("max", ctypes.c_uint64, 8),
        ("volt", ctypes.c_uint64, 8),
        ("pwr", ctypes.c_uint64, 8),
        ("pstate", ctypes.c_uint64, 5),
        ("reserved_45", ctypes.c_uint64, 1),
        ("realm", ctypes.c_uint64, 1),
        ("change", ctypes.c_uint64, 1),
        ("mpam_id_low", ctypes.c_uint64, 8),
        ("mpam_id_high", ctypes.c_uint64, 4),
        ("cstate", ctypes.c_uint64, 3),
    ]


class sensor_data(ctypes.Union):
    _fields_ = [
        ("current", current_data),
        ("voltage", volt_data),
        ("temp0", temp_full_data0),
        ("max_temp", temp_max_data),
        ("as_uint64", ctypes.c_uint64),
    ]


class sensor_telem(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("data", sensor_data),
        ("temp1", temp_full_data1),
        ("temp2", temp_full_data2),
    ]


class pstate_data(ctypes.Structure):
    _fields_ = [
        ("pstate", ctypes.c_uint64, 5),
        ("reserved_5_7", ctypes.c_uint64, 3),
        ("throttle_status", ctypes.c_uint64, 4),
        ("vm_throttle_pri", ctypes.c_uint64, 4),
        ("base_perf_vfsm_pn", ctypes.c_uint64, 5),
        ("reserved_23_21", ctypes.c_uint64, 3),
        ("max_pstate", ctypes.c_uint64, 5),
        ("reserved_29", ctypes.c_uint64, 1),
        ("cstate", ctypes.c_uint64, 3),
        ("reserved_33", ctypes.c_uint64, 1),
        ("plimit", ctypes.c_uint64, 5),
        ("reserved_39", ctypes.c_uint64, 1),
        ("core", ctypes.c_uint64, 7),
        ("overflow", ctypes.c_uint64, 1),
        ("mpam_low", ctypes.c_uint64, 8),
        ("mpam_high", ctypes.c_uint64, 4),
        ("boost_priority", ctypes.c_uint64, 4),

    ]


class pstate_telem(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("data", pstate_data),
    ]


class plimit_data_msg(ctypes.Structure):
    _fields_ = [
        ("type", ctypes.c_uint16, 2),
        ("reserved_2_3", ctypes.c_uint16, 2),
        ("mpam_high", ctypes.c_uint16, 4),
        ("pstate", ctypes.c_uint16, 5),
        ("reserved_15_13", ctypes.c_uint16, 3),
        ("byte2_3_as_uint16", ctypes.c_uint16),
        ("plimit", ctypes.c_uint32, 5),
        ("reserved_39_37", ctypes.c_uint32, 3),
        ("core_id", ctypes.c_uint32, 7),
        ("overflow", ctypes.c_uint32, 1),
        ("mpam_low", ctypes.c_uint32, 8),
        ("reserved_58_56", ctypes.c_uint32, 3),
        ("s_desired_perf", ctypes.c_uint32, 5),
    ]


class plimit_telem_msg(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("data", plimit_data_msg),
    ]


# Enums
class sensor_fifo_id(IntEnum):
    sensor_fifo_pstate_telemetry_hw = 0
    sensor_fifo_scp_msg_telemetry_hw = 1
    sensor_fifo_tile_temperature_telemetry_hw = 2
    sensor_fifo_tile_voltage_telemetry_hw = 3
    sensor_fifo_core_current_telemetry_hw = 4
    sensor_fifo_pvt_temp_fw = 5
    sensor_fifo_pvt_voltage_fw = 6
    sensor_fifo_dimm_temp_fw = 7
    sensor_fifo_vr_temp_fw = 8
    sensor_fifo_vr_current_fw = 9
    sensor_fifo_max_id = 10


# C Structures as Python ctypes Structures
class sensor_fifo_properties(ctypes.Structure):
    _fields_ = [
        ("entry_size_bytes", ctypes.c_uint16),
        ("stride_size_bytes", ctypes.c_uint16),
        ("start_address_incl", ctypes.c_uint32),  # inclusive
        ("end_address_excl", ctypes.c_uint32),  # exclusive, last address + 1
        ("num_entries_or_strides", ctypes.c_uint16),  # 1-indexed
        ("name", ctypes.c_char_p),
    ]


class tile_temp(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("temp0", temp_full_data0),  # Reference to temp_full_data0 structure
        ("temp1", temp_full_data1),  # Reference to temp_full_data1 structure
        ("temp2", temp_full_data2),  # Reference to temp_full_data2 structure
    ]


class tile_voltage(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("data", volt_data),  # Reference to volt_data structure
    ]


class core_current(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("data", current_data),  # Reference to current_data structure
    ]


class soc_pvt_temp(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("sensor_temp_dc", ctypes.c_uint16 * 15),  # Array of 15 uint16 values
        ("padding", ctypes.c_uint8 * 2),  # Padding to align to 8 bytes
    ]


class soc_pvt_voltage(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("sensor_voltage_mv", ctypes.c_uint16 * 18),  # Array of 18 uint16 values
        ("padding", ctypes.c_uint8 * 4),  # Padding to align to 8 bytes
    ]


class vr_temp(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("vr_temp_dc", ctypes.c_uint16 * 8),  # Array of 8 uint16 values
    ]


class vr_current(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("vr_current_ma", ctypes.c_uint16 * 8),  # Array of 8 uint16 values
        ("vr_voltage_mv", ctypes.c_uint16 * 8),  # Array of 8 uint16 values
    ]


class sensor_ram_dimm_info(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("dimm_temp_s0_dc", ctypes.c_uint16),
        ("dimm_temp_s1_dc", ctypes.c_uint16),
        ("dimm_power_mw", ctypes.c_uint16),
        ("dimm_id", ctypes.c_uint8),
        ("dimm_throttling", ctypes.c_uint8),  # 1 = throttle, 0 = not throttle
        ("dimm_memory_frequency_id", ctypes.c_uint8),
        ("padding", ctypes.c_uint8 * 7),  # Padding to align to 8 bytes
    ]

