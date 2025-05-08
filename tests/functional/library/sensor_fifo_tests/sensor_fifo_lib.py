from dataclasses import dataclass
from enum import IntEnum
from typing import List, Union, Dict, Optional, Tuple
import struct
import time

# Constants from header files
NUMBER_OF_SOC_TEMP_SENSORS = 15
NUMBER_OF_SOC_VOLT_MON_SENSORS = 18
MAX_NUM_OF_VR_RAILS = 8
QUADWORD_SIZE = 8
FIFO_TIMESTAMP_SIZE = QUADWORD_SIZE

class SensorTelemType(IntEnum):
    """Maps to SENSOR_TELEM_TYPE enum"""
    TEMPERATURE = 0
    VOLTAGE = 1
    CURRENT = 2

class PstateThrottleStatusCodes(IntEnum):
    """Maps to PSTATE_THROTTLE_STATUS_CODES enum"""
    NO_THROTTLE = 0
    CURRENT_THROTTLE_START = 1
    TEMP_THROTTLE_START = 2
    RACK_THROTTLE_START = 3
    SYS_FRC_PMIN_THROTTLE_START = 4
    ADPT_CLK_THROTTLE_START = 5
    CURRENT_THROTTLE_END = 6
    TEMP_THROTTLE_END = 7
    RACK_THROTTLE_END = 8
    SYS_FRC_PMIN_THROTTLE_END = 9
    ADPT_CLK_THROTTLE_END = 10
    CURRENT_THROTTLE_OVERRUN = 11
    ADPT_CLK_THROTTLE_OVERRUN = 12

class SensorFifoId(IntEnum):
    """Maps to SENSOR_FIFO_ID enum"""
    PSTATE_TELEMETRY_HW = 0
    SCP_MSG_TELEMETRY_HW = 1
    TILE_TEMPERATURE_TELEMETRY_HW = 2
    TILE_VOLTAGE_TELEMETRY_HW = 3
    CORE_CURRENT_TELEMETRY_HW = 4
    PVT_TEMP_FW = 5
    PVT_VOLTAGE_FW = 6
    DIMM_TEMP_FW = 7
    VR_TEMP_FW = 8
    VR_CURRENT_FW = 9
    MAX_ID = 10

@dataclass
class FifoProperties:
    """Maps to sensor_fifo_properties_t struct"""
    entry_size_bytes: int
    stride_size_bytes: int
    start_address_incl: int
    end_address_excl: int
    num_entries_or_strides: int
    name: str

class SensorDataCreator:
    """Helper class to create sensor FIFO data"""

    @staticmethod
    def get_timestamp() -> int:
        """Get current timestamp in nanoseconds"""
        return int(time.time() * 1e9)

    @staticmethod
    def create_pstate_data(
        timestamp: Optional[int] = None,
        pstate: int = 0,
        throttle_status: int = 0,
        vm_throttle_pri: int = 0,
        base_perf_vfsm_pn: int = 0,
        max_pstate: int = 0,
        cstate: int = 0,
        plimit: int = 0,
        core: int = 0,
        overflow: bool = False,
        mpam_low: int = 0,
        mpam_high: int = 0,
        boost_priority: int = 0
    ) -> List[str]:
        """Create pstate telemetry data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        # Validate inputs
        if not (0 <= pstate <= 31): raise ValueError("pstate must be 0-31")
        if not (0 <= throttle_status <= 15): raise ValueError("throttle_status must be 0-15")
        if not (0 <= vm_throttle_pri <= 15): raise ValueError("vm_throttle_pri must be 0-15")
        if not (0 <= base_perf_vfsm_pn <= 31): raise ValueError("base_perf_vfsm_pn must be 0-31")
        if not (0 <= max_pstate <= 31): raise ValueError("max_pstate must be 0-31")
        if not (0 <= cstate <= 7): raise ValueError("cstate must be 0-7")
        if not (0 <= plimit <= 31): raise ValueError("plimit must be 0-31")
        if not (0 <= core <= 135): raise ValueError("core must be 0-135")
        if not (0 <= mpam_low <= 255): raise ValueError("mpam_low must be 0-255")
        if not (0 <= mpam_high <= 15): raise ValueError("mpam_high must be 0-15")
        if not (0 <= boost_priority <= 15): raise ValueError("boost_priority must be 0-15")

        # Pack data according to pstate_data_t structure
        pstate_data = (
            (pstate & 0x1F) |
            ((throttle_status & 0xF) << 8) |
            ((vm_throttle_pri & 0xF) << 12) |
            ((base_perf_vfsm_pn & 0x1F) << 16) |
            ((max_pstate & 0x1F) << 24) |
            ((cstate & 0x7) << 30) |
            ((plimit & 0x1F) << 34) |
            ((core & 0x7F) << 40) |
            ((1 if overflow else 0) << 47) |
            ((mpam_low & 0xFF) << 48) |
            ((mpam_high & 0xF) << 56) |
            ((boost_priority & 0xF) << 60)
        )

        return [
            f"0x{timestamp:016x}",
            f"0x{pstate_data:016x}"
        ]

    @staticmethod
    def create_scp_message(
        timestamp: Optional[int] = None,
        msg_type: int = 0,
        mpam_high: int = 0,
        pstate: int = 0,
        ldo_voltage: Optional[int] = None,
        cppc_desired: Optional[int] = None,
        throttle_pri: Optional[int] = None,
        boost_pri: Optional[int] = None,
        desired_perf: Optional[int] = None,
        plimit: int = 0,
        core_id: int = 0,
        overflow: bool = False,
        mpam_low: int = 0,
        s_desired_perf: int = 0
    ) -> List[str]:
        """Create SCP message telemetry data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        # Validate inputs
        if not (0 <= msg_type <= 3): raise ValueError("msg_type must be 0-3")
        if not (0 <= mpam_high <= 15): raise ValueError("mpam_high must be 0-15")
        if not (0 <= pstate <= 31): raise ValueError("pstate must be 0-31")
        if ldo_voltage is not None and not (0 <= ldo_voltage <= 511):
            raise ValueError("ldo_voltage must be 0-511")
        if cppc_desired is not None and not (0 <= cppc_desired <= 31):
            raise ValueError("cppc_desired must be 0-31")
        if not (0 <= plimit <= 31): raise ValueError("plimit must be 0-31")
        if not (0 <= core_id <= 135): raise ValueError("core_id must be 0-135")
        if not (0 <= mpam_low <= 255): raise ValueError("mpam_low must be 0-255")
        if not (0 <= s_desired_perf <= 31): raise ValueError("s_desired_perf must be 0-31")

        # Pack first 16 bits
        data = (msg_type & 0x3) | ((mpam_high & 0xF) << 4) | ((pstate & 0x1F) << 8)

        # Pack second 16 bits based on message type
        if msg_type == 0:  # plimit success message
            if ldo_voltage is None or cppc_desired is None:
                raise ValueError("ldo_voltage and cppc_desired required for plimit success message")
            data |= ((ldo_voltage & 0x1FF) << 16) | ((cppc_desired & 0x1F) << 27)
        else:  # cppc update message
            if throttle_pri is None or boost_pri is None or desired_perf is None:
                raise ValueError("throttle_pri, boost_pri, and desired_perf required for cppc update message")
            data |= ((throttle_pri & 0xF) << 16) | ((boost_pri & 0xF) << 20) | ((desired_perf & 0x1F) << 24)

        # Pack remaining bits
        data |= ((plimit & 0x1F) << 32) | ((core_id & 0x7F) << 39) | \
                ((1 if overflow else 0) << 46) | ((mpam_low & 0xFF) << 47) | \
                ((s_desired_perf & 0x1F) << 59)

        return [
            f"0x{timestamp:016x}",
            f"0x{data:016x}"
        ]

    @staticmethod
    def create_tile_temperature(
        timestamp: Optional[int] = None,
        temp_valid: int = 1,
        max_id: int = 0,
        max_temp: int = 0,
        core0_temp: int = 0,
        core1_temp: int = 0,
        sensor_temps: List[int] = None
    ) -> List[str]:
        """Create tile temperature telemetry data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        if sensor_temps is None:
            sensor_temps = [0] * 8
        elif len(sensor_temps) != 8:
            raise ValueError("sensor_temps must contain exactly 8 values")

        # Validate temperature ranges (assuming 16-bit temperatures)
        for temp in [max_temp, core0_temp, core1_temp] + sensor_temps:
            if not (0 <= temp <= 0xFFFF):
                raise ValueError("Temperature values must be 0-65535")

        # Pack data according to tile_temp_t structure
        temp0_data = (
            (temp_valid & 0xFF) |
            ((max_id & 0xFF) << 8) |
            ((max_temp & 0xFFFF) << 16) |
            ((core0_temp & 0xFFFF) << 32) |
            ((core1_temp & 0xFFFF) << 48)
        )

        temp1_data = (
            (sensor_temps[0] & 0xFFFF) |
            ((sensor_temps[1] & 0xFFFF) << 16) |
            ((sensor_temps[2] & 0xFFFF) << 32) |
            ((sensor_temps[3] & 0xFFFF) << 48)
        )

        temp2_data = (
            (sensor_temps[4] & 0xFFFF) |
            ((sensor_temps[5] & 0xFFFF) << 16) |
            ((sensor_temps[6] & 0xFFFF) << 32) |
            ((sensor_temps[7] & 0xFFFF) << 48)
        )

        return [
            f"0x{timestamp:016x}",
            f"0x{temp0_data:016x}",
            f"0x{temp1_data:016x}",
            f"0x{temp2_data:016x}"
        ]

    @staticmethod
    def create_tile_voltage(
        timestamp: Optional[int] = None,
        vcore0: int = 0,
        vcore1: int = 0,
        vcpu: int = 0,
        vsys: int = 0
    ) -> List[str]:
        """Create tile voltage telemetry data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        # Validate voltage ranges (16-bit values)
        for voltage in [vcore0, vcore1, vcpu, vsys]:
            if not (0 <= voltage <= 0xFFFF):
                raise ValueError("Voltage values must be 0-65535")

        # Pack data according to volt_data_t structure
        voltage_data = (
            (vcore0 & 0xFFFF) |
            ((vcore1 & 0xFFFF) << 16) |
            ((vcpu & 0xFFFF) << 32) |
            ((vsys & 0xFFFF) << 48)
        )

        return [
            f"0x{timestamp:016x}",
            f"0x{voltage_data:016x}"
        ]

    @staticmethod
    def create_core_current(
        timestamp: Optional[int] = None,
        avg: int = 0,
        min_val: int = 0,
        max_val: int = 0,
        voltage: int = 0,
        power: int = 0,
        pstate: int = 0,
        realm: bool = False,
        change: bool = False,
        mpam_id_low: int = 0,
        mpam_id_high: int = 0,
        cstate: int = 0
    ) -> List[str]:
        """Create core current telemetry data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        # Validate input ranges
        if not (0 <= avg <= 0xFF): raise ValueError("avg must be 0-255")
        if not (0 <= min_val <= 0xFF): raise ValueError("min_val must be 0-255")
        if not (0 <= max_val <= 0xFF): raise ValueError("max_val must be 0-255")
        if not (0 <= voltage <= 0xFF): raise ValueError("voltage must be 0-255")
        if not (0 <= power <= 0xFF): raise ValueError("power must be 0-255")
        if not (0 <= pstate <= 31): raise ValueError("pstate must be 0-31")
        if not (0 <= mpam_id_low <= 0xFF): raise ValueError("mpam_id_low must be 0-255")
        if not (0 <= mpam_id_high <= 0xF): raise ValueError("mpam_id_high must be 0-15")
        if not (0 <= cstate <= 7): raise ValueError("cstate must be 0-7")

        # Pack data according to current_data_t structure
        current_data = (
            (avg & 0xFF) |
            ((min_val & 0xFF) << 8) |
            ((max_val & 0xFF) << 16) |
            ((voltage & 0xFF) << 24) |
            ((power & 0xFF) << 32) |
            ((pstate & 0x1F) << 40) |
            ((1 if realm else 0) << 46) |
            ((1 if change else 0) << 47) |
            ((mpam_id_low & 0xFF) << 48) |
            ((mpam_id_high & 0xF) << 56) |
            ((cstate & 0x7) << 60)
        )

        return [
            f"0x{timestamp:016x}",
            f"0x{current_data:016x}"
        ]

    @staticmethod
    def create_pvt_temperature(
        timestamp: Optional[int] = None,
        temperatures: List[int] = None
    ) -> List[str]:
        """Create PVT temperature telemetry data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        if temperatures is None:
            temperatures = [0] * NUMBER_OF_SOC_TEMP_SENSORS
        elif len(temperatures) != NUMBER_OF_SOC_TEMP_SENSORS:
            raise ValueError(f"temperatures must contain exactly {NUMBER_OF_SOC_TEMP_SENSORS} values")

        # Validate temperature ranges (16-bit values)
        for temp in temperatures:
            if not (0 <= temp <= 0xFFFF):
                raise ValueError("Temperature values must be 0-65535")

        result = [f"0x{timestamp:016x}"]

        # Pack temperatures into quadwords (4 temperatures per quadword)
        for i in range(0, len(temperatures), 4):
            chunk = temperatures[i:i+4]
            while len(chunk) < 4:  # Pad last chunk if needed
                chunk.append(0)

            data = 0
            for j, temp in enumerate(chunk):
                data |= (temp & 0xFFFF) << (j * 16)
            result.append(f"0x{data:016x}")

        # Add padding if needed to make total size multiple of QUADWORD_SIZE
        if len(result) * 8 % QUADWORD_SIZE != 0:
            result.append("0x0000000000000000")

        return result

    @staticmethod
    def create_pvt_voltage(
        timestamp: Optional[int] = None,
        voltages: List[int] = None
    ) -> List[str]:
        """Create PVT voltage telemetry data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        if voltages is None:
            voltages = [0] * NUMBER_OF_SOC_VOLT_MON_SENSORS
        elif len(voltages) != NUMBER_OF_SOC_VOLT_MON_SENSORS:
            raise ValueError(f"voltages must contain exactly {NUMBER_OF_SOC_VOLT_MON_SENSORS} values")

        # Validate voltage ranges (16-bit values)
        for voltage in voltages:
            if not (0 <= voltage <= 0xFFFF):
                raise ValueError("Voltage values must be 0-65535")

        result = [f"0x{timestamp:016x}"]

        # Pack voltages into quadwords (4 voltages per quadword)
        for i in range(0, len(voltages), 4):
            chunk = voltages[i:i+4]
            while len(chunk) < 4:  # Pad last chunk if needed
                chunk.append(0)

            data = 0
            for j, voltage in enumerate(chunk):
                data |= (voltage & 0xFFFF) << (j * 16)
            result.append(f"0x{data:016x}")

        # Add padding if needed
        if len(result) * 8 % QUADWORD_SIZE != 0:
            result.append("0x0000000000000000")

        return result

    @staticmethod
    def create_dimm_info(
        timestamp: Optional[int] = None,
        dimm_id: int = 0,
        dimm_throttling: int = 0,
        dimm_memory_frequency_id: int = 0,
        dimm_temp_s0: int = 0,
        dimm_temp_s1: int = 0,
        dimm_power: int = 0,
        dimm_mr4_throttle_count: int = 0
    ) -> List[str]:
        """Create DIMM information data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        # Validate input ranges
        if not (0 <= dimm_id <= 0xFF): raise ValueError("dimm_id must be 0-255")
        if not (0 <= dimm_throttling <= 0xFF): raise ValueError("dimm_throttling must be 0-255")
        if not (0 <= dimm_memory_frequency_id <= 0xFF): raise ValueError("dimm_memory_frequency_id must be 0-255")

        # Validate 16-bit values
        for value in [dimm_temp_s0, dimm_temp_s1, dimm_power, dimm_mr4_throttle_count]:
            if not (0 <= value <= 0xFFFF):
                raise ValueError("Temperature and power values must be 0-65535")

        # Pack first quadword: timestamp
        result = [f"0x{timestamp:016x}"]

        # Pack second quadword:
        data1 = (
            (dimm_mr4_throttle_count & 0xFFFF) |
            ((dimm_power & 0xFFFF) << 16) |
            ((dimm_temp_s1 & 0xFFFF) << 32) |
            ((dimm_temp_s0 & 0xFFFF) << 48)
        )
        result.append(f"0x{data1:016x}")

        # Pack third quadword: thresholds
        data2 = (
            0 |
            ((dimm_memory_frequency_id & 0xFF) << 40) |
            ((dimm_throttling & 0xFF) << 48) |
            ((dimm_id & 0xFF) << 56)
        )
        result.append(f"0x{data2:016x}")

        return result

    @staticmethod
    def create_vr_temperature(
        timestamp: Optional[int] = None,
        temperatures: List[int] = None
    ) -> List[str]:
        """Create VR temperature telemetry data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        if temperatures is None:
            temperatures = [0] * MAX_NUM_OF_VR_RAILS
        elif len(temperatures) != MAX_NUM_OF_VR_RAILS:
            raise ValueError(f"temperatures must contain exactly {MAX_NUM_OF_VR_RAILS} values")

        # Validate temperature ranges (16-bit values)
        for temp in temperatures:
            if not (0 <= temp <= 0xFFFF):
                raise ValueError("Temperature values must be 0-65535")

        result = [f"0x{timestamp:016x}"]

        # Pack temperatures into quadwords (4 temperatures per quadword)
        for i in range(0, len(temperatures), 4):
            chunk = temperatures[i:i+4]
            while len(chunk) < 4:  # Pad last chunk if needed
                chunk.append(0)

            data = 0
            for j, temp in enumerate(chunk):
                data |= (temp & 0xFFFF) << (j * 16)
            result.append(f"0x{data:016x}")

        return result

    @staticmethod
    def create_vr_current(
        timestamp: Optional[int] = None,
        currents: List[int] = None,
        voltages: List[int] = None
    ) -> List[str]:
        """Create VR current and voltage telemetry data"""
        if timestamp is None:
            timestamp = SensorDataCreator.get_timestamp()

        if currents is None:
            currents = [0] * MAX_NUM_OF_VR_RAILS
        if voltages is None:
            voltages = [0] * MAX_NUM_OF_VR_RAILS

        if len(currents) != MAX_NUM_OF_VR_RAILS or len(voltages) != MAX_NUM_OF_VR_RAILS:
            raise ValueError(f"currents and voltages must contain exactly {MAX_NUM_OF_VR_RAILS} values each")

        # Validate ranges (16-bit values)
        for value in currents + voltages:
            if not (0 <= value <= 0xFFFF):
                raise ValueError("Current and voltage values must be 0-65535")

        result = [f"0x{timestamp:016x}"]

        # Pack currents and voltages into quadwords
        for i in range(0, MAX_NUM_OF_VR_RAILS, 2):
            data = (
                (currents[i] & 0xFFFF) |
                ((voltages[i] & 0xFFFF) << 16) |
                ((currents[i+1] & 0xFFFF) << 32) |
                ((voltages[i+1] & 0xFFFF) << 48)
            )
            result.append(f"0x{data:016x}")

        return result

def create_sensor_data(fifo_id: Union[SensorFifoId, int], timestamp_offset_ns: int = 0, **kwargs) -> List[str]:
    """Creates FIFO data based on the FIFO ID and provided parameters

    Args:
        fifo_id: The FIFO ID to create data for
        timestamp_offset_ns: Optional offset in nanoseconds to add to the timestamp (default: 0)
        **kwargs: Additional parameters specific to each FIFO type
    """
    fifo_id = SensorFifoId(fifo_id)

    # If timestamp is provided in kwargs, use it, otherwise generate new one with offset
    if 'timestamp' not in kwargs:
        kwargs['timestamp'] = SensorDataCreator.get_timestamp() + timestamp_offset_ns

    creators = {
        SensorFifoId.PSTATE_TELEMETRY_HW: SensorDataCreator.create_pstate_data,
        SensorFifoId.SCP_MSG_TELEMETRY_HW: SensorDataCreator.create_scp_message,
        SensorFifoId.TILE_TEMPERATURE_TELEMETRY_HW: SensorDataCreator.create_tile_temperature,
        SensorFifoId.TILE_VOLTAGE_TELEMETRY_HW: SensorDataCreator.create_tile_voltage,
        SensorFifoId.CORE_CURRENT_TELEMETRY_HW: SensorDataCreator.create_core_current,
        SensorFifoId.PVT_TEMP_FW: SensorDataCreator.create_pvt_temperature,
        SensorFifoId.PVT_VOLTAGE_FW: SensorDataCreator.create_pvt_voltage,
        SensorFifoId.DIMM_TEMP_FW: SensorDataCreator.create_dimm_info,
        SensorFifoId.VR_TEMP_FW: SensorDataCreator.create_vr_temperature,
        SensorFifoId.VR_CURRENT_FW: SensorDataCreator.create_vr_current
    }

    if fifo_id not in creators:
        raise ValueError(f"Unsupported FIFO ID: {fifo_id}")

    return creators[fifo_id](**kwargs)

# Example usage
if __name__ == "__main__":
    # Example: Create temperature FIFO data
    temp_data = create_sensor_data(
        SensorFifoId.TILE_TEMPERATURE_TELEMETRY_HW,
        temp_valid=1,
        max_id=2,
        max_temp=80,
        core0_temp=75,
        core1_temp=78,
        sensor_temps=[70, 72, 74, 76, 71, 73, 75, 77]
    )
    print("Temperature Data:", temp_data)

    # Example: Create voltage FIFO data
    voltage_data = create_sensor_data(
        SensorFifoId.TILE_VOLTAGE_TELEMETRY_HW,
        vcore0=850,
        vcore1=855,
        vcpu=1000,
        vsys=1200
    )
    print("Voltage Data:", voltage_data)

    # Example: Create VR data
    vr_data = create_sensor_data(
        SensorFifoId.VR_CURRENT_FW,
        currents=[100, 110, 120, 130, 140, 150, 160, 170],
        voltages=[1000, 1010, 1020, 1030, 1040, 1050, 1060, 1070]
    )
    print("VR Data:", vr_data)