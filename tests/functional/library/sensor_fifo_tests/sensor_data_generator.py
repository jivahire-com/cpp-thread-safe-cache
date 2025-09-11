# Copyright (c) Microsoft Corporation. All rights reserved.

from typing import List, Optional, Union, Dict
import random
from sensor_fifo_lib import (
    SensorFifoId,
    create_sensor_data,
    NUMBER_OF_SOC_TEMP_SENSORS,
    NUMBER_OF_SOC_VOLT_MON_SENSORS,
    MAX_NUM_OF_VR_RAILS,
    PstateThrottleStatusCodes
)

class SensorDataGenerator:
    """Generator for random sensor FIFO data"""

    # Robot Framework FIFO name mapping
    FIFO_NAME_TO_ID = {
        "PSTATE": SensorFifoId.PSTATE_TELEMETRY_HW,
        "SCP_MSG": SensorFifoId.SCP_MSG_TELEMETRY_HW,
        "TEMPERATURE": SensorFifoId.TILE_TEMPERATURE_TELEMETRY_HW,
        "VOLTAGE": SensorFifoId.TILE_VOLTAGE_TELEMETRY_HW,
        "CURRENT": SensorFifoId.CORE_CURRENT_TELEMETRY_HW,
        "PVT_TEMP": SensorFifoId.PVT_TEMP_FW,
        "PVT_VOLTAGE": SensorFifoId.PVT_VOLTAGE_FW,
        "DIMM_TEMP": SensorFifoId.DIMM_TEMP_FW,
        "VR_TEMP": SensorFifoId.VR_TEMP_FW,
        "VR_CURRENT": SensorFifoId.VR_CURRENT_FW
    }

    def __init__(self, seed: Optional[int] = None):
        """Initialize generator with optional random seed"""
        if seed is not None:
            random.seed(seed)

        # Define reasonable ranges for different sensor types
        self.ranges = {
            'temperature': (20, 95),    # °C
            'voltage': {
                'vcore': (750, 950),    # mV
                'vcpu': (900, 1100),    # mV
                'vsys': (1100, 1300),   # mV
                'ldo': (0, 511),        # Raw value
                'pvt': (0, 0xFFFF)      # 0-65535 for PVT_VOLTAGE values
            },
            'current': (50, 200),       # mA
            'power': (10, 100),         # W
            'pstate': {
                'value': (0, 31),       # P-state values
                'throttle_status': (0, 12),  # Changed from enum to direct value
                'vm_throttle_pri': (0, 15),
                'base_perf': (0, 31),
                'cstate': (0, 7),
                'plimit': (0, 31),
                'core': (0, 135),
                'mpam_low': (0, 255),
                'mpam_high': (0, 15),
                'boost_priority': (0, 15)
            },
            'scp_msg': {
                'msg_type': (0, 3),
                'cppc_desired': (0, 31),
                'throttle_pri': (0, 15),
                'boost_pri': (0, 15),
                'desired_perf': (0, 31),
                's_desired_perf': (0, 31)
            },
            'dimm': {
                'id': (0, 6),            # DIMM IDs
                'freq': (0, 10),         # DDRSS_SPEED_GRADE
                'throttle': (0, 3),      # Throttling states
                'mr4_count' : (0, 1000), # dimm_mr4_throttle_count
                'duration_ms': (0, 255)  # dimm throttle duration mS
            }
        }

    def get_valid_core_id(self) -> int:
        """Get a valid core_id value within the range 0-135.

        Returns:
            int: A core_id value between 0 and 135
        """
        return random.randint(0, 135)  # Direct generation within valid range

    def generate_random_data(self, fifo_id: Union[SensorFifoId, int], num_entries: int) -> List[List[str]]:
        """Generate specified number of random entries for given FIFO type"""
        fifo_id = SensorFifoId(fifo_id)
        entries = []

        timestamp_base = random.randint(0, 3600) * int(1e9)  # Random base within 24 hours

        for entry_num in range(num_entries):
            timestamp_offset = timestamp_base + (entry_num * random.randint(1000000, 999999999))
            if fifo_id == SensorFifoId.PSTATE_TELEMETRY_HW:
                entry = create_sensor_data(
                    fifo_id,
                    timestamp_offset_ns=timestamp_offset,
                    pstate=random.randint(*self.ranges['pstate']['value']),
                    throttle_status=random.randint(*self.ranges['pstate']['throttle_status']),
                    vm_throttle_pri=random.randint(*self.ranges['pstate']['vm_throttle_pri']),
                    base_perf_vfsm_pn=random.randint(*self.ranges['pstate']['base_perf']),
                    max_pstate=random.randint(*self.ranges['pstate']['value']),
                    cstate=random.randint(*self.ranges['pstate']['cstate']),
                    plimit=random.randint(*self.ranges['pstate']['plimit']),
                    core=self.get_valid_core_id(),  # Use clamped core_id
                    overflow=random.choice([True, False]),
                    mpam_low=random.randint(*self.ranges['pstate']['mpam_low']),
                    mpam_high=random.randint(*self.ranges['pstate']['mpam_high']),
                    boost_priority=random.randint(*self.ranges['pstate']['boost_priority'])
                )

            elif fifo_id == SensorFifoId.SCP_MSG_TELEMETRY_HW:
                msg_type = random.randint(*self.ranges['scp_msg']['msg_type'])
                valid_core_id = self.get_valid_core_id()  # Get valid core_id before creating data

                try:
                    entry = create_sensor_data(
                        fifo_id,
                        timestamp_offset_ns=timestamp_offset,
                        msg_type=msg_type,
                        mpam_high=random.randint(*self.ranges['pstate']['mpam_high']),
                        pstate=random.randint(*self.ranges['pstate']['value']),
                        ldo_voltage=random.randint(*self.ranges['voltage']['ldo']) if msg_type == 0 else None,
                        cppc_desired=random.randint(*self.ranges['scp_msg']['cppc_desired']) if msg_type == 0 else None,
                        throttle_pri=random.randint(*self.ranges['scp_msg']['throttle_pri']) if msg_type != 0 else None,
                        boost_pri=random.randint(*self.ranges['scp_msg']['boost_pri']) if msg_type != 0 else None,
                        desired_perf=random.randint(*self.ranges['scp_msg']['desired_perf']) if msg_type != 0 else None,
                        plimit=random.randint(*self.ranges['pstate']['plimit']),
                        core_id=valid_core_id,  # Use pre-validated core_id
                        overflow=random.choice([True, False]),
                        mpam_low=random.randint(*self.ranges['pstate']['mpam_low']),
                        s_desired_perf=random.randint(*self.ranges['scp_msg']['s_desired_perf'])
                    )
                except ValueError as e:
                    print(f"Debug - core_id: {valid_core_id}, msg_type: {msg_type}")  # Debug info
                    raise e

            elif fifo_id == SensorFifoId.TILE_TEMPERATURE_TELEMETRY_HW:
                entry = create_sensor_data(
                    fifo_id,
                    timestamp_offset_ns=timestamp_offset,
                    temp_valid=1,
                    max_id=random.randint(0, 7),
                    max_temp=random.randint(*self.ranges['temperature']),
                    core0_temp=random.randint(*self.ranges['temperature']),
                    core1_temp=random.randint(*self.ranges['temperature']),
                    sensor_temps=[random.randint(*self.ranges['temperature']) for _ in range(8)]
                )

            elif fifo_id == SensorFifoId.TILE_VOLTAGE_TELEMETRY_HW:
                entry = create_sensor_data(
                    fifo_id,
                    timestamp_offset_ns=timestamp_offset,
                    vcore0=random.randint(*self.ranges['voltage']['vcore']),
                    vcore1=random.randint(*self.ranges['voltage']['vcore']),
                    vcpu=random.randint(*self.ranges['voltage']['vcpu']),
                    vsys=random.randint(*self.ranges['voltage']['vsys'])
                )

            elif fifo_id == SensorFifoId.CORE_CURRENT_TELEMETRY_HW:
                current = random.randint(*self.ranges['current'])
                voltage_mv = random.randint(*self.ranges['voltage']['vcore'])
                voltage_scaled = min(255, int(voltage_mv / 4))  # Scale down voltage to 0-255 range
                entry = create_sensor_data(
                    fifo_id,
                    timestamp_offset_ns=timestamp_offset,
                    avg=current,
                    min_val=max(current - 10, self.ranges['current'][0]),
                    max_val=min(current + 10, self.ranges['current'][1]),
                    voltage=voltage_scaled,  # Use scaled voltage value
                    power=random.randint(*self.ranges['power']),
                    pstate=random.randint(*self.ranges['pstate']['value']),
                    realm=random.choice([True, False]),
                    change=random.choice([True, False]),
                    mpam_id_low=random.randint(*self.ranges['pstate']['mpam_low']),
                    mpam_id_high=random.randint(*self.ranges['pstate']['mpam_high']),
                    cstate=random.randint(*self.ranges['pstate']['cstate'])
                )

            elif fifo_id == SensorFifoId.PVT_TEMP_FW:
                entry = create_sensor_data(
                    fifo_id,
                    timestamp_offset_ns=timestamp_offset,
                    temperatures=[random.randint(*self.ranges['temperature'])
                                for _ in range(NUMBER_OF_SOC_TEMP_SENSORS)]
                )

            elif fifo_id == SensorFifoId.PVT_VOLTAGE_FW:
                entry = create_sensor_data(
                    fifo_id,
                    timestamp_offset_ns=timestamp_offset,  # Use unique timestamp
                    voltages=[random.randint(*self.ranges['voltage']['vcore'])
                             for _ in range(NUMBER_OF_SOC_VOLT_MON_SENSORS)]
                )

            elif fifo_id == SensorFifoId.DIMM_TEMP_FW:
                entry = create_sensor_data(
                    fifo_id,
                    timestamp_offset_ns=timestamp_offset,
                    dimm_id=random.randint(*self.ranges['dimm']['id']),
                    dimm_throttling=random.randint(*self.ranges['dimm']['throttle']),
                    dimm_memory_frequency_id=random.randint(*self.ranges['dimm']['freq']),
                    dimm_temp_s0=random.randint(*self.ranges['temperature']),
                    dimm_temp_s1=random.randint(*self.ranges['temperature']),
                    dimm_power=random.randint(*self.ranges['power']),
                    dimm_mr4_throttle_count=random.randint(*self.ranges['dimm']['mr4_count']),
                    dimm_throttle_duration_ms=random.randint(*self.ranges['dimm']['duration_ms']),
                )

            elif fifo_id == SensorFifoId.VR_TEMP_FW:
                entry = create_sensor_data(
                    fifo_id,
                    timestamp_offset_ns=timestamp_offset,
                    temperatures=[random.randint(*self.ranges['temperature'])
                                for _ in range(MAX_NUM_OF_VR_RAILS)]
                )

            elif fifo_id == SensorFifoId.VR_CURRENT_FW:
                entry = create_sensor_data(
                    fifo_id,
                    timestamp_offset_ns=timestamp_offset,
                    currents=[random.randint(*self.ranges['current'])
                             for _ in range(MAX_NUM_OF_VR_RAILS)],
                    voltages=[random.randint(*self.ranges['voltage']['vcore'])
                             for _ in range(MAX_NUM_OF_VR_RAILS)]
                )

            else:
                raise ValueError(f"Unsupported FIFO ID: {fifo_id}")

            entries.append(entry)

        return entries

    def generate_sensor_fifo_data(self, fifo_name: str, num_entries: int = 1, seed: Optional[int] = None) -> List[List[str]]:
        """Generate random sensor FIFO data entries for the specified FIFO type"""
        fifo_name = fifo_name.upper()
        if fifo_name not in self.FIFO_NAME_TO_ID:
            raise ValueError(f"Unknown FIFO type: {fifo_name}. Valid types: {list(self.FIFO_NAME_TO_ID.keys())}")

        if seed is not None:
            random.seed(seed)

        fifo_id = self.FIFO_NAME_TO_ID[fifo_name]

        return self.generate_random_data(fifo_id, num_entries)

    def generate_all_sensor_fifo_data(self, num_entries: int = 1, seed: Optional[int] = None) -> Dict[str, List[List[str]]]:
        """Generate random sensor FIFO data entries for all FIFO types"""
        if seed is not None:
            random.seed(seed)

        result = {}
        for fifo_name in self.FIFO_NAME_TO_ID:
            result[fifo_name] = self.generate_sensor_fifo_data(fifo_name, num_entries)
        return result

def generate_sensor_data(
    fifo_id: Union[SensorFifoId, int],
    num_entries: int,
    seed: Optional[int] = None
) -> List[List[str]]:
    """Convenience function to generate random sensor data"""
    generator = SensorDataGenerator(seed)
    return generator.generate_random_data(fifo_id, num_entries)

if __name__ == "__main__":
    # Example usage with seed for reproducibility
    SEED = 42
    generator = SensorDataGenerator(SEED)

    # Example 1: Generate specific FIFO type data
    temp_data = generator.generate_sensor_fifo_data("TEMPERATURE", num_entries=3)
    print("\n=== Temperature Data ===")
    for i, entry in enumerate(temp_data, 1):
        print(f"\nEntry {i}:")
        for qword in entry:
            print(f"  {qword}")

    # Example 2: Test PVT_VOLTAGE specifically
    pvt_voltage_data = generator.generate_sensor_fifo_data("PVT_VOLTAGE", num_entries=2)
    print("\n=== PVT_VOLTAGE Data ===")
    for i, entry in enumerate(pvt_voltage_data, 1):
        print(f"\nEntry {i}:")
        for j, qword in enumerate(entry):
            print(f"  Buffer[{j}]: {qword}")

    # Example 3: Generate all FIFO types
    print("\n=== Generating All FIFO Types ===")
    all_data = generator.generate_all_sensor_fifo_data(num_entries=4)
    for fifo_name, entries in all_data.items():
        print(f"\n{fifo_name}:")
        for i, entry in enumerate(entries, 1):
            print(f"Entry {i}:")
            for qword in entry:
                print(f"  {qword}")