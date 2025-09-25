# Copyright (c) Microsoft Corporation. All rights reserved.
import logging
import os
import sys
import subprocess
import time
from enum import IntEnum
from fpfw_automation_primitives.serial.telnet import Telnet_  # noqa: F401
from pathlib import Path


# Add paths for both package and direct imports
mts_dir = str(Path(__file__).parent.parent / "mts_tests")
icc_dir = str(Path(__file__).parent.parent / "icc_mhu_tests")
current_dir = os.path.dirname(os.path.abspath(__file__))
pylibs_dir = os.path.dirname(current_dir)
# Only add paths if they're not already present
paths_to_add = [mts_dir, pylibs_dir, current_dir]
for path in paths_to_add:
    if path not in sys.path:
        sys.path.append(path)


try:
    from .dcp_protocol import data_collection_protocol
    from .dcp_commands import dcp_commands
    from .icc_mhu_ap_lib import ApCore, IccMhuAp
    from .trp_lib import (  # noqa: F401
        trp_endpoint,
        mts_cli_trp_endpoint,
        mts_ap_endpoint,
    )
    from .trp_protocol import transfer_relay_protocol

except ImportError:
    from dcp_protocol import data_collection_protocol
    from dcp_commands import dcp_commands
    from icc_mhu_ap_lib import ApCore, IccMhuAp
    from trp_lib import (  # noqa: F401
        trp_endpoint,
        mts_cli_trp_endpoint,
        mts_ap_endpoint,
    )
    from trp_protocol import transfer_relay_protocol

logger = logging.getLogger(__name__)


# Constants for DCP protocol
POWER_PKG_TELEMETRY_PROVIDER_ID = 0x0201
INST_PKG_TELEMETRY_PROVIDER_ID = 0x0202

# Polling sleep duration in seconds (5 milliseconds)
READ_PACKAGE_DURATION_SEC = 0.005


class pwr_telemetry_element_id_t(IntEnum):
    """Power telemetry element identifiers (16-bit values)"""

    POWER_TELEMETRY_ELEMENT_CORE_PSTATE = 0
    POWER_TELEMETRY_ELEMENT_CORE_CSTATE = 1
    POWER_TELEMETRY_ELEMENT_CORE_THROTTLE = 2
    POWER_TELEMETRY_ELEMENT_CORE_RACK_PRIORITIES = 3
    POWER_TELEMETRY_ELEMENT_CORE_VOLTAGE = 4
    POWER_TELEMETRY_ELEMENT_CORE_CURRENT = 5
    POWER_TELEMETRY_ELEMENT_CORE_TEMPERATURE = 6
    POWER_TELEMETRY_ELEMENT_CORE_POWER = 7
    POWER_TELEMETRY_ELEMENT_CORE_HISTOGRAM = 8
    POWER_TELEMETRY_ELEMENT_CORE_AGING = 9
    POWER_TELEMETRY_ELEMENT_CORE_DROOPS = 10
    POWER_TELEMETRY_ELEMENT_SOC_PKG_MON = 11
    POWER_TELEMETRY_ELEMENT_SOC_VR_RAILS = 12
    POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE = 13
    POWER_TELEMETRY_ELEMENT_SOC_DIMM_POWER = 14
    POWER_TELEMETRY_ELEMENT_SOC_HNF_TEMP = 15
    POWER_TELEMETRY_ELEMENT_SOC_SENSOR_TEMP = 16
    POWER_TELEMETRY_ELEMENT_SOC_PER_DIE_MESH = 17
    POWER_TELEMETRY_ELEMENT_SOC_DIE_TO_DIE_LINK_STATE = 18
    POWER_TELEMETRY_ELEMENT_SOC_MAX_TEMPERATURE = 19
    POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_CORE_POWER = 20
    POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_THROTTLE = 21
    POWER_TELEMETRY_ELEMENT_SOC_VM_MPAM_MEMORY_POWER = 22
    POWER_TELEMETRY_ELEMENT_ID_MAX = 23


class instantaneous_telemetry_element_id_t(IntEnum):
    """Instantaneous telemetry element identifiers (16-bit values)"""

    INST_TELEMETRY_ELEMENT_CORE = 0
    INST_TELEMETRY_ELEMENT_SOC_VOLTAGE_RAILS = 1
    INST_TELEMETRY_ELEMENT_SOC_DIMM_RT = 2
    INST_TELEMETRY_ELEMENT_SOC_DIE_TEMP = 3
    INST_TELEMETRY_ELEMENT_SOC_MAX_TEMP = 4
    INST_TELEMETRY_ELEMENT_ID_MAX = 5


class power_tlm_lib:

    def __init__(self, src_endpoint: trp_endpoint, file_location: str):
        """
        Initialize the power telemetry library with a TRP endpoint.

        This class provides an interface for managing power telemetry data collection
        through the Data Collection Protocol (DCP) over a Transfer Relay Protocol (TRP)
        endpoint connection.

        Args:
            src_endpoint (trp_endpoint): The TRP endpoint object used for communication
                with the power telemetry client. This endpoint handles the low-level
                protocol communication for DCP commands.
            file_location (str): Directory path where telemetry files will be stored.
                If the directory exists, it will be cleared. If it doesn't exist,
                it will be created.

        Attributes:
            src_endpoint (trp_endpoint): Stored reference to the TRP endpoint
            file_location (str): Directory path for telemetry files
            manifest_file (str): Path or name of the telemetry manifest file (empty by default)
            tlm_packages (list): List to store telemetry packages retrieved from the client

        Raises:
            OSError: If the directory cannot be created or cleared
        """
        self.src_endpoint = src_endpoint

        # Handle file_location directory validation and management
        if os.path.exists(file_location):
            if os.path.isdir(file_location):
                # Directory exists, clear its contents
                try:
                    import shutil

                    for filename in os.listdir(file_location):
                        file_path = os.path.join(file_location, filename)
                        if os.path.isfile(file_path) or os.path.islink(file_path):
                            os.unlink(file_path)
                        elif os.path.isdir(file_path):
                            shutil.rmtree(file_path)
                except OSError as e:
                    raise OSError(f"Failed to clear directory '{file_location}': {e}")
            else:
                raise OSError(f"Path '{file_location}' exists but is not a directory")
        else:
            # Directory doesn't exist, attempt to create it
            try:
                os.makedirs(file_location, exist_ok=True)
            except OSError as e:
                raise OSError(f"Failed to create directory '{file_location}': {e}")

        self.file_location = file_location
        self.manifest_file = ""
        self.tlm_packages = []

    def reset(self):
        """
        Reset the DCP (Data Collection Protocol) power telemetry client.

        Sends a reset command to the power telemetry client on the MCP
        (Management Control Processor) to restore it to its initial state. This command
        clears any active data collection sessions, resets internal client state,
        and prepares the client for new operations.

        The command targets:
        - Destination die: 0 (primary die)
        - Destination CPU: MCP (Management Control Processor)
        - Client ID: Power Instantaneous Telemetry client

        Raises:
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        dcp_commands.client_reset(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
        )

    def read_manifest(self, filename: str):
        """
        Read and retrieve the telemetry manifest file from the DCP service.

        This method requests the manifest file from the Data Collection Protocol (DCP)
        service and saves it to the specified location. The manifest file contains
        metadata about the telemetry data format, structure, and element definitions
        required for decoding binary telemetry packages.

        Args:
            filename (str): The name of the manifest file to create in the file_location
                directory. Common convention is to use "manifest.bin" for binary manifests.

        Attributes Updated:
            self.manifest_file (str): Set to the full path of the downloaded manifest file

        Raises:
            Exception: If the DCP command fails, communication with the endpoint fails,
                or the manifest file cannot be written to the specified location

        Note:
            The manifest is retrieved from the DCP service client, not the power telemetry
            client, as indicated by the MTS_CLIENT_ID_DCP_SVC client ID.
        """
        self.manifest_file = os.path.join(self.file_location, filename)

        status, response = dcp_commands.client_get_manifest(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_DCP_SVC,
            output_file=self.manifest_file,
        )

    def enable_power_package_records(self, elements: list[pwr_telemetry_element_id_t]):
        """
        Enable power package recording for specified telemetry elements.

        This method enables data collection for specific power telemetry elements
        by sending enable/disable events commands to the DCP client.

        Args:
            elements (list[pwr_telemetry_element_id_t]): List of power telemetry elements
                to enable for data collection.

        Raises:
            ValueError: If the elements list is empty or contains invalid elements
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        if not elements:
            raise ValueError("Elements list cannot be empty")

        # Convert pwr_telemetry_element_id_t enum values to DCP event tuples
        # Format: (provider_id, event_id, state)
        # Using element enum value as event_id, with ENABLE state
        event_tuple = []
        enable_state = (
            data_collection_protocol.client_events_enable_disable_msg.dcp_events_enable_state_t.DCP_EVENTS_ENABLE_STATE_ENABLE  # noqa: E501
        )

        for element in elements:
            if not isinstance(element, pwr_telemetry_element_id_t):
                raise ValueError(
                    f"Invalid element type: {type(element)}. Expected pwr_telemetry_element_id_t"
                )

            # Create event tuple: (provider_id, event_id, enable_state)
            # Using POWER_PKG_TELEMETRY_PROVIDER_ID for power telemetry events
            # Using element enum value as event_id
            event_tuple.append(
                (
                    POWER_PKG_TELEMETRY_PROVIDER_ID,  # Provider ID for power telemetry
                    element.value,  # Event ID from enum
                    enable_state,
                )
            )

        elements_list = "\n".join([f"  - {elem.name}" for elem in elements])
        logger.info(
            f"Enabling {len(event_tuple)} power telemetry elements:\n{elements_list}"
        )

        # Send the enable/disable events command
        dcp_commands.client_enable_disable_events(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
            events=event_tuple,
        )

    def enable_all_power_package_records(self):
        """
        Enable power package recording for all available telemetry elements.

        This method creates a list of all power telemetry elements (excluding the MAX value)
        and enables data collection for all of them by calling enable_power_package_records.

        Raises:
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        # Generate list of all power telemetry elements (excluding the MAX element)
        all_elements = [
            element
            for element in pwr_telemetry_element_id_t
            if element != pwr_telemetry_element_id_t.POWER_TELEMETRY_ELEMENT_ID_MAX
        ]

        # Enable all elements using the existing method
        self.enable_power_package_records(all_elements)

    def disable_power_package_records(self, elements: list[pwr_telemetry_element_id_t]):
        """
        Disable power package recording for specified telemetry elements.

        This method disables data collection for specific power telemetry elements
        by sending enable/disable events commands to the DCP client.

        Args:
            elements (list[pwr_telemetry_element_id_t]): List of power telemetry elements
                to disable for data collection.

        Raises:
            ValueError: If the elements list is empty or contains invalid elements
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        if not elements:
            raise ValueError("Elements list cannot be empty")

        # Convert pwr_telemetry_element_id_t enum values to DCP event tuples
        # Format: (provider_id, event_id, state)
        # Using element enum value as event_id, with DISABLE state
        event_tuple = []
        disable_state = (
            data_collection_protocol.client_events_enable_disable_msg.dcp_events_enable_state_t.DCP_EVENTS_ENABLE_STATE_DISABLE  # noqa: E501
        )

        for element in elements:
            if not isinstance(element, pwr_telemetry_element_id_t):
                raise ValueError(
                    f"Invalid element type: {type(element)}. Expected pwr_telemetry_element_id_t"
                )

            # Create event tuple: (provider_id, event_id, disable_state)
            # Using POWER_PKG_TELEMETRY_PROVIDER_ID for power telemetry events
            # Using element enum value as event_id
            event_tuple.append(
                (
                    POWER_PKG_TELEMETRY_PROVIDER_ID,  # Provider ID for power telemetry
                    element.value,  # Event ID from enum
                    disable_state,
                )
            )

        elements_list = "\n".join([f"  - {elem.name}" for elem in elements])
        logger.info(
            f"Disabling {len(event_tuple)} power telemetry elements:\n{elements_list}"
        )

        # Send the enable/disable events command
        dcp_commands.client_enable_disable_events(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
            events=event_tuple,
        )

    def disable_all_power_package_records(self):
        """
        Disable power package recording for all available telemetry elements.

        This method creates a list of all power telemetry elements (excluding the MAX value)
        and disables data collection for all of them by calling disable_power_package_records.

        Raises:
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        # Generate list of all power telemetry elements (excluding the MAX element)
        all_elements = [
            element
            for element in pwr_telemetry_element_id_t
            if element != pwr_telemetry_element_id_t.POWER_TELEMETRY_ELEMENT_ID_MAX
        ]

        # Disable all elements using the existing method
        self.disable_power_package_records(all_elements)

    def enable_instantaneous_package_records(
        self, elements: list[instantaneous_telemetry_element_id_t]
    ):
        """
        Enable instantaneous package recording for specified telemetry elements.

        This method enables data collection for specific instantaneous telemetry elements
        by sending enable/disable events commands to the DCP client.

        Args:
            elements (list[instantaneous_telemetry_element_id_t]): List of instantaneous telemetry elements
                to enable for data collection.

        Raises:
            ValueError: If the elements list is empty or contains invalid elements
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        if not elements:
            raise ValueError("Elements list cannot be empty")

        # Convert instantaneous_telemetry_element_id_t enum values to DCP event tuples
        # Format: (provider_id, event_id, state)
        # Using element enum value as event_id, with ENABLE state
        event_tuple = []
        enable_state = (
            data_collection_protocol.client_events_enable_disable_msg.dcp_events_enable_state_t.DCP_EVENTS_ENABLE_STATE_ENABLE  # noqa: E501
        )

        for element in elements:
            if not isinstance(element, instantaneous_telemetry_element_id_t):
                raise ValueError(
                    f"Invalid element type: {type(element)}. Expected instantaneous_telemetry_element_id_t"
                )

            # Create event tuple: (provider_id, event_id, enable_state)
            # Using INST_PKG_TELEMETRY_PROVIDER_ID for instantaneous telemetry events
            # Using element enum value as event_id
            event_tuple.append(
                (
                    INST_PKG_TELEMETRY_PROVIDER_ID,  # Provider ID for instantaneous telemetry
                    element.value,  # Event ID from enum
                    enable_state,
                )
            )

        elements_list = "\n".join([f"  - {elem.name}" for elem in elements])
        logger.info(
            f"Enabling {len(event_tuple)} instantaneous telemetry elements:\n{elements_list}"
        )

        # Send the enable/disable events command
        dcp_commands.client_enable_disable_events(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
            events=event_tuple,
        )

    def enable_all_instantaneous_package_records(self):
        """
        Enable instantaneous package recording for all available telemetry elements.

        This method creates a list of all instantaneous telemetry elements (excluding the MAX value)
        and enables data collection for all of them by calling enable_instantaneous_package_records.

        Raises:
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        # Generate list of all instantaneous telemetry elements (excluding the MAX element)
        all_elements = [
            element
            for element in instantaneous_telemetry_element_id_t
            if element
            != instantaneous_telemetry_element_id_t.INST_TELEMETRY_ELEMENT_ID_MAX
        ]

        # Enable all elements using the existing method
        self.enable_instantaneous_package_records(all_elements)

    def disable_instantaneous_package_records(
        self, elements: list[instantaneous_telemetry_element_id_t]
    ):
        """
        Disable instantaneous package recording for specified telemetry elements.

        This method disables data collection for specific instantaneous telemetry elements
        by sending enable/disable events commands to the DCP client.

        Args:
            elements (list[instantaneous_telemetry_element_id_t]): List of instantaneous telemetry elements
                to disable for data collection.

        Raises:
            ValueError: If the elements list is empty or contains invalid elements
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        if not elements:
            raise ValueError("Elements list cannot be empty")

        # Convert instantaneous_telemetry_element_id_t enum values to DCP event tuples
        # Format: (provider_id, event_id, state)
        # Using element enum value as event_id, with DISABLE state
        event_tuple = []
        disable_state = (
            data_collection_protocol.client_events_enable_disable_msg.dcp_events_enable_state_t.DCP_EVENTS_ENABLE_STATE_DISABLE  # noqa: E501
        )

        for element in elements:
            if not isinstance(element, instantaneous_telemetry_element_id_t):
                raise ValueError(
                    f"Invalid element type: {type(element)}. Expected instantaneous_telemetry_element_id_t"
                )

            # Create event tuple: (provider_id, event_id, disable_state)
            # Using INST_PKG_TELEMETRY_PROVIDER_ID for instantaneous telemetry events
            # Using element enum value as event_id
            event_tuple.append(
                (
                    INST_PKG_TELEMETRY_PROVIDER_ID,  # Provider ID for instantaneous telemetry
                    element.value,  # Event ID from enum
                    disable_state,
                )
            )

        elements_list = "\n".join([f"  - {elem.name}" for elem in elements])
        logger.info(
            f"Disabling {len(event_tuple)} instantaneous telemetry elements:\n{elements_list}"
        )

        # Send the enable/disable events command
        dcp_commands.client_enable_disable_events(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
            events=event_tuple,
        )

    def disable_all_instantaneous_package_records(self):
        """
        Disable instantaneous package recording for all available telemetry elements.

        This method creates a list of all instantaneous telemetry elements (excluding the MAX value)
        and disables data collection for all of them by calling disable_instantaneous_package_records.

        Raises:
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        # Generate list of all instantaneous telemetry elements (excluding the MAX element)
        all_elements = [
            element
            for element in instantaneous_telemetry_element_id_t
            if element
            != instantaneous_telemetry_element_id_t.INST_TELEMETRY_ELEMENT_ID_MAX
        ]

        # Disable all elements using the existing method
        self.disable_instantaneous_package_records(all_elements)

    def get_client_state(
        self,
    ) -> data_collection_protocol.client_get_state_msg.dcp_client_state_t:
        """
        Get the current running state of the DCP power telemetry client.

        Sends a command to query the current state of the power telemetry client on the MCP
        (Management Control Processor). The state indicates whether the client is currently
        running, stopped, or in an error state.

        Returns:
            data_collection_protocol.client_get_state_msg.dcp_client_state_t: The current state of the client.
        Raises:
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        state = dcp_commands.client_get_state(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
        )
        return state

    def start_dcp_client(self):
        """
        Start the DCP (Data Collection Protocol) power telemetry client.

        Sends a start command to the power telemetry client on the MCP
        (Management Control Processor) to begin data collection operations. This must
        be called before attempting to read telemetry data from the client.

        The command targets:
        - Destination die: 0 (primary die)
        - Destination CPU: MCP (Management Control Processor)
        - Client ID: Power Instantaneous Telemetry client
        - State: START

        Raises:
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        dcp_commands.client_start_stop(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
            state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_START,
        )

    def stop_dcp_client(self):
        """
        Stop the DCP (Data Collection Protocol) power telemetry client.

        Sends a stop command to the power telemetry client on the MCP
        (Management Control Processor) to halt data collection operations. This should
        be called when telemetry collection is no longer needed to properly clean up
        resources and stop the client.

        The command targets:
        - Destination die: 0 (primary die)
        - Destination CPU: MCP (Management Control Processor)
        - Client ID: Power Instantaneous Telemetry client
        - State: STOP

        Raises:
            Exception: If the DCP command fails or communication with the endpoint fails
        """
        dcp_commands.client_start_stop(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
            state=data_collection_protocol.client_start_stop_msg.dcp_start_stop_state_t.DCP_START_STOP_STATE_STOP,
        )

    def read_packages_for_duration(
        self, filename_prefix: str, duration_sec: int = 30
    ) -> None:
        """
        Read telemetry packages for a specified duration from the DCP client.

        This method continuously reads telemetry packages for the specified duration,
        saving each package to a separate file with the specified filename prefix.
        The method will collect as many packages as are available during the time frame
        without raising exceptions when the duration expires. Successfully read package
        filenames are added to self.tlm_packages.

        Args:
            filename_prefix (str): Prefix for output filenames. Files will be named as
                "{prefix}_package_{index}.bin" where index starts from 1
            duration_sec (int, optional): Duration to read packages for in seconds.
                Defaults to 30 seconds.

        Raises:
            ValueError: If filename_prefix is empty
            Exception: If DCP commands fail or communication with the endpoint fails

        Note:
            Successfully read package filenames are automatically added to self.tlm_packages
            for later processing or reference. No exception is raised when the duration
            expires - this is normal completion.

        Example:
            # Read packages for 60 seconds with prefix "telemetry_data"
            pwr_tlm.read_packages_for_duration("telemetry_data", 60)
            # Check results: print(f"Read {len(pwr_tlm.tlm_packages)} packages in 60 seconds")
        """
        self._read_packages_loop(
            filename_prefix, max_packages=None, duration_sec=duration_sec
        )

    def read_n_packages(
        self, num_packages: int, filename_prefix: str, timeout_sec: int = 30
    ) -> None:
        """
        Read a specified number of telemetry packages from the DCP client.

        This method reads multiple telemetry packages sequentially, saving each package
        to a separate file with the specified filename prefix. The method will continue
        reading packages until either the requested number is reached, no more data is
        available, or the timeout is exceeded. Successfully read package filenames are
        added to self.tlm_packages.

        Args:
            num_packages (int): Number of packages to read (must be positive)
            filename_prefix (str): Prefix for output filenames. Files will be named as
                "{prefix}_package_{index}.bin" where index starts from 1
            timeout_sec (int, optional): Total timeout for the entire operation in seconds.
                Defaults to 30 seconds.

        Raises:
            ValueError: If num_packages is not positive or filename_prefix is empty
            TimeoutError: If the operation times out before completing
            Exception: If DCP commands fail or communication with the endpoint fails

        Note:
            Successfully read package filenames are automatically added to self.tlm_packages
            for later processing or reference.

        Example:
            # Read 5 packages with prefix "telemetry_data"
            pwr_tlm.read_n_packages(5, "telemetry_data", 60)
            # Creates files: telemetry_data_package_1.bin, telemetry_data_package_2.bin, etc.
            # Check results: print(f"Read {len(pwr_tlm.tlm_packages)} packages")
        """
        # Input validation
        if num_packages <= 0:
            raise ValueError(f"num_packages must be positive, got {num_packages}")

        self._read_packages_loop(
            filename_prefix, max_packages=num_packages, duration_sec=timeout_sec
        )

    def decode_to_file(
        self, manifest_path, payload_path, output_path, capture_output=True, check=True
    ):
        exe_path = f"{os.environ['REPO_APP_PATH_1psfw.diagnosticdecoder.windows']}/tools/x64/diagdecoder.exe"
        cmd = [
            exe_path,
            manifest_path,
            payload_path,
            output_path,
        ]

        result = subprocess.run(cmd, capture_output=capture_output, check=check)
        if result.returncode != 0:
            logger.error(
                f"DiagnosticDecoder failed: {result.stderr.decode() if result.stderr else f'Unknown error - stdout: {result.stdout}'}"  # noqa: E501
            )
            logger.error(
                "Ensure that the latest MSCV C++ Redistributable is installed: https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version"  # noqa: E501
            )
            raise subprocess.CalledProcessError(
                result.returncode, cmd, output=result.stdout, stderr=result.stderr
            )

    def decode_packages(self):
        """
        Decode telemetry packages using the manifest file.

        This method processes telemetry data files stored in self.tlm_packages,
        using the manifest file to decode the binary data into JSON format.

        Raises:
            FileNotFoundError: If the manifest file doesn't exist
            ValueError: If no telemetry packages are available for decoding
        """
        # Check if manifest file exists
        if not self.manifest_file or not os.path.exists(self.manifest_file):
            raise FileNotFoundError(f"Manifest file not found: {self.manifest_file}")

        # Check if there are any telemetry packages to process
        if not self.tlm_packages:
            raise ValueError(
                "No telemetry packages available for decoding. "
                "Use read_n_packages or read_packages_for_duration first."
            )

        # Process each telemetry package file
        for package_filepath in self.tlm_packages:
            if not os.path.exists(package_filepath):
                logger.warning(f"Package file not found, skipping: {package_filepath}")
                continue

            # Create output filename by replacing the suffix with .json
            base_name = os.path.splitext(package_filepath)[0]
            output_file = f"{base_name}.json"

            logger.info(f"Processing file: {package_filepath} -> {output_file}")

            self.decode_to_file(
                manifest_path=self.manifest_file,
                payload_path=package_filepath,
                output_path=output_file,
            )

    def _read_dcp_package(
        self, output_file: str
    ) -> data_collection_protocol.dcp_status_t:
        """
        Private helper function to read a single data package from the DCP client.

        This method handles the low-level DCP data reading protocol:
        1. Sends a read_data command to the power telemetry client
        2. Checks if the response contains valid data
        3. Logs the data package information at debug level
        4. Sends a read_data_complete acknowledgment

        Args:
            output_file (str): The file path where the DCP data package will be written

        Returns:
            data_collection_protocol.dcp_status_t: The DCP status indicating data availability

        Raises:
            Exception: If the DCP command fails or communication with the endpoint fails

        Note:
            This is a private method intended for internal use by other data reading methods.
            It handles the protocol-level details of DCP data reading operations.
            Package information is logged for debugging but not returned.
        """
        status, response = dcp_commands.client_read_data(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
            output_file=output_file,
        )

        # Check if we received valid data
        if status not in [
            data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_LAST,
            data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_MORE,
        ]:
            return status

        # Send read data complete acknowledgment
        complete_status = dcp_commands.client_send_read_data_complete(
            src_endpoint=self.src_endpoint,
            dest_die=0,
            dest_cpu=transfer_relay_protocol.cpu_type.CPU_MCP,
            client_id=data_collection_protocol.mts_client_id_t.MTS_CLIENT_ID_PWR_INST_TELEM,
            rd_data_addr_offset=response.rd_data_addr_offset,
            rd_data_size=response.rd_data_size,
        )

        # Handle edge cases: If complete_status is NONE, we know a package was read
        # (since client_read_data succeeded), so return VALID_LAST. Otherwise trust complete_status.
        if (
            complete_status
            == data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_NONE
        ):
            return (
                data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_LAST
            )

        return complete_status

    def _read_packages_loop(
        self,
        filename_prefix: str,
        max_packages: int | None = None,
        duration_sec: int = 30,
    ) -> int:
        """
        Private helper method to read packages in a loop with flexible termination conditions.

        This method contains the common logic for reading multiple telemetry packages.
        It can terminate based on either a maximum number of packages or a duration timeout.

        Args:
            filename_prefix (str): Prefix for output filenames
            max_packages (int | None, optional): Maximum number of packages to read. If None,
                will read until duration expires.
            duration_sec (int, optional): Maximum duration to read for in seconds.
                Defaults to 30 seconds.

        Returns:
            int: Number of packages successfully read

        Raises:
            TimeoutError: If max_packages is specified and timeout occurs before reaching the target
            Exception: If DCP commands fail or communication with the endpoint fails

        Note:
            Successfully read package filenames are automatically added to self.tlm_packages
        """
        if not filename_prefix or not filename_prefix.strip():
            raise ValueError("filename_prefix cannot be empty")

        created_files = []
        start_time = time.time()
        package_index = 1

        # Determine termination condition message
        if max_packages is not None:
            target_msg = f"{max_packages} packages"
        else:
            target_msg = f"{duration_sec} seconds"

        logger.info(
            f"Starting to read packages for {target_msg} with prefix '{filename_prefix}'"
        )

        try:
            while True:
                # Check duration timeout
                elapsed_time = time.time() - start_time
                if elapsed_time >= duration_sec:
                    if max_packages is not None:
                        # Only raise timeout error if we were trying to reach a specific count
                        raise TimeoutError(
                            f"Timeout ({duration_sec}s) exceeded after reading {len(created_files)} packages"
                        )
                    else:
                        # Duration expired - this is normal completion for duration-based reading
                        break

                # Check package count limit
                if max_packages is not None and len(created_files) >= max_packages:
                    break

                # Create filename for this package
                package_filename = f"{filename_prefix}_package_{package_index}.bin"
                package_filepath = os.path.join(self.file_location, package_filename)

                if max_packages is not None:
                    logger.debug(
                        f"Attempting to read package {package_index} "
                        f"(have {len(created_files)}/{max_packages}) to '{package_filepath}'"
                    )
                else:
                    logger.debug(
                        f"Attempting to read package {package_index} "
                        f"(have {len(created_files)}, {elapsed_time:.1f}s/{duration_sec}s) to '{package_filepath}'"
                    )

                # Read the package
                status = self._read_dcp_package(package_filepath)

                # Check if the package was successfully read
                if status in [
                    data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_LAST,
                    data_collection_protocol.dcp_status_t.DATA_COLLECTION_RD_DATA_VALID_MORE,
                ]:
                    created_files.append(package_filepath)
                    # Add to the instance's tlm_packages list
                    self.tlm_packages.append(package_filepath)
                    logger.debug(
                        f"Successfully read package {package_index}: {status.name}"
                    )
                    # Only increment package_index when we successfully read a package
                    package_index += 1

                # Sleep briefly between iterations to avoid excessive polling
                time.sleep(READ_PACKAGE_DURATION_SEC)

            logger.info(
                f"Completed reading {len(created_files)} packages in {time.time() - start_time:.2f} seconds"
            )
            logger.info(f"Total packages in tlm_packages: {len(self.tlm_packages)}")
            return len(created_files)

        except Exception as e:
            logger.error(f"Error during package reading: {e}")
            # Clean up any partially created files if desired
            # Note: Keeping files for debugging purposes
            raise


# to run the main functionality below from a repo environment
# & (Join-Path ([System.Environment]::GetEnvironmentVariable("REPO_APP_PATH_python.win64", "Process")) "/tools/python.exe") tests/functional/library/power_telemetry_tests/pwr_tlm_lib.py  # noqa: E501
#
def main():
    logging.basicConfig(
        level=logging.DEBUG,  # Set the logging level (DEBUG, INFO, etc.)
        handlers=[logging.StreamHandler(sys.stdout)],  # Redirect to stdout
    )

    # Reduce spam from external libraries
    logging.getLogger("fpfw_automation_primitives.serial.telnet").setLevel(
        logging.WARNING
    )
    logging.getLogger("fpfw_automation_primitives").setLevel(logging.WARNING)

    # Keep our application loggers at detailed level
    logging.getLogger(__name__).setLevel(logging.DEBUG)
    logging.getLogger("dcp_commands").setLevel(logging.DEBUG)
    logging.getLogger("dcp_protocol").setLevel(logging.DEBUG)
    logging.getLogger("icc_mhu_ap_lib").setLevel(logging.DEBUG)
    logging.getLogger("trp_lib").setLevel(logging.DEBUG)

    # Telnet Endpoint - Die 0 SCP - Uncomment if using SVP
    # telnet_port = Telnet_(host="localhost", port="4257", encoding="UTF-8")
    # telnet_port.open()

    # mts_endpoint = mts_cli_trp_endpoint(
    #     telnet_port, 0, transfer_relay_protocol.cpu_type.CPU_SCP
    # )

    # ICC MHU Endpoint - Die 0 AP 0 - Comment out if not using Trace32
    mts_endpoint = mts_ap_endpoint()

    pwr_tlm = power_tlm_lib(mts_endpoint, "C:/scratch/lib")

    exception_occurred = False

    try:
        pwr_tlm.reset()
        pwr_tlm.read_manifest("manifest.bin")

        # # Enable specific power telemetry elements
        pwr_elements_to_enable = [
            pwr_telemetry_element_id_t.POWER_TELEMETRY_ELEMENT_CORE_CURRENT,
            pwr_telemetry_element_id_t.POWER_TELEMETRY_ELEMENT_SOC_DIMM_TEMPERATURE,
        ]
        pwr_tlm.enable_power_package_records(pwr_elements_to_enable)

        pwr_tlm.start_dcp_client()

        state = pwr_tlm.get_client_state()
        logger.info(f"Current DCP client state: {state.name} ({state.value})")

        pwr_tlm.read_n_packages(6, "pwr_records", 65 * 10)
    except Exception as e:
        logger.error(f"Exception occurred during telemetry operations: {e}")
        exception_occurred = True
        raise
    finally:
        # Always attempt to stop the DCP client
        try:
            pwr_tlm.stop_dcp_client()
            logger.info("DCP client stopped successfully")
        except Exception as stop_e:
            logger.error(f"Failed to stop DCP client: {stop_e}")

    # Only decode packages if no exceptions occurred
    if not exception_occurred:
        try:
            pwr_tlm.decode_packages()
        except Exception as decode_e:
            logger.error(f"Failed to decode packages: {decode_e}")
            raise


if __name__ == "__main__":
    main()
